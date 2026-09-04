/*
 * Copyright (C) 2026 CharOfString <root@charofstring.cc>
 *
 * This file is part of FlakeWM.
 *
 * FlakeWM is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * FlakeWM is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * FlakeWM. If not, see <https://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 * Referenced:
 *   - waylib/examples/outputviewport/main.cpp: output/render lifecycle.
 *   - waylib/examples/tinywl/helper.cpp: seat, xdg-shell, socket and backend.
 *   - the reference FlakeWM compositor supplied for this implementation.
 */

#include <wcursor.h>
#include <woutputrenderwindow.h>
#include <woutputviewport.h>
#include <wquickoutputlayout.h>
#include <wrenderhelper.h>
#include <wseat.h>
#include <wsocket.h>
#include <wsurfaceitem.h>
#include <wxdgpopupsurface.h>
#include <wxdgpopupsurfaceitem.h>
#include <wxdgshell.h>
#include <wxdgtoplevelsurface.h>
#include <wxdgtoplevelsurfaceitem.h>

#include <wlr_all.h>

#include <QDebug>
#include <QHash>
#include <QInputEvent>
#include <QJSValue>
#include <QPointer>
#include <WBackend>
#include <WOutput>
#include <WSeat>
#include <WServer>

#include "src/core/compositor/compositor.h"

class FlakeCompositor::SeatEventFilter final : public WSeatEventFilter {
 public:
  explicit SeatEventFilter(FlakeCompositor* compositor)
      : WSeatEventFilter(compositor), compositor_(compositor) {}

  void Register(WXdgToplevelSurface* toplevel) {
    if (!states_.contains(toplevel)) {
      states_.insert(toplevel, ToplevelState{});
    }

    connect(toplevel, &WToplevelSurface::requestMove, this,
        [this, toplevel](WSeat* seat, quint32) {
      if (seat != compositor_->seat_) {
        return;
      }
      auto it = states_.find(toplevel);
      if (it == states_.end() || !it->item || it->minimized) {
        return;
      }

      if (it->maximized) {
        RestoreForMove(toplevel, *it);
      }

      moving_toplevel_ = toplevel;
      move_start_cursor_ = compositor_->cursor_->position();
      move_start_position_ = it->item->position();
    });

    connect(toplevel, &WToplevelSurface::requestMaximize, this,
        [this, toplevel]() { Maximize(toplevel); });
    connect(toplevel, &WToplevelSurface::requestCancelMaximize, this,
        [this, toplevel]() { Unmaximize(toplevel); });
    connect(toplevel, &WToplevelSurface::requestMinimize, this,
        [this, toplevel]() { Minimize(toplevel); });
  }

  void BindItem(WXdgToplevelSurface* toplevel, WSurfaceItem* item) {
    auto it = states_.find(toplevel);
    if (it == states_.end()) {
      it = states_.insert(toplevel, ToplevelState{});
    }
    it->item = item;
    surface_items_.insert(toplevel->surface(), item);
    item->setFocusPolicy(Qt::StrongFocus);
  }

  void BindPopupItem(WXdgPopupSurface* popup,
      WXdgPopupSurfaceItem* item) {
    WSurface* parent_surface = popup->parentSurface();
    WSurfaceItem* parent_item = surface_items_.value(parent_surface);
    if (parent_item) {
      item->setParentItem(parent_item);
      item->setZ(1000);

      auto update_position = [item, parent_item]() {
        if (!item || !parent_item || !parent_item->shellSurface()) {
          return;
        }

        // wlr_xdg_popup_get_position() returns surface-local coordinates and
        // therefore includes the parent's xdg_surface geometry offset. A
        // WSurfaceItem, however, is positioned at the content-geometry origin.
        // Remove that offset or CSD clients (GTK commonly uses e.g. 25,12 for
        // its shadow extents) leave the popup visibly detached from its anchor.
        const QPoint geometry_offset =
          parent_item->shellSurface()->getContentGeometry().topLeft();
        item->setPosition(item->implicitPosition()
          - QPointF(geometry_offset));
      };
      connect(item, &WXdgPopupSurfaceItem::implicitPositionChanged,
          item, update_position);
      update_position();
    }
    surface_items_.insert(popup->surface(), item);
  }

  void RemovePopup(WXdgPopupSurface* popup) {
    surface_items_.remove(popup->surface());
  }

  void Activate(WSeat* seat, WXdgToplevelSurface* toplevel,
      WSurfaceItem* surface_item = nullptr) {
    // Match Treeland's SurfaceWrapper::setActivate(): sending the same
    // activated state again schedules another xdg_toplevel.configure. Doing
    // that between a pointer press and release cancels GTK click gestures.
    if (active_toplevel_ != toplevel) {
      if (active_toplevel_ && active_toplevel_->isInitialized()) {
        active_toplevel_->setActivate(false);
      }
      active_toplevel_ = toplevel;
      if (active_toplevel_ && active_toplevel_->isInitialized()) {
        active_toplevel_->setActivate(true);
      }
    }

    if (surface_item && !surface_item->hasActiveFocus()) {
      surface_item->forceActiveFocus(Qt::MouseFocusReason);
    }
    seat->setKeyboardFocusSurface(
      active_toplevel_ ? active_toplevel_->surface() : nullptr);
  }

  void Remove(WSeat* seat, WXdgToplevelSurface* toplevel) {
    if (moving_toplevel_ == toplevel) {
      moving_toplevel_.clear();
    }
    if (active_toplevel_ == toplevel) {
      if (active_toplevel_->isInitialized()) {
        active_toplevel_->setActivate(false);
      }
      active_toplevel_.clear();
      seat->setKeyboardFocusSurface(nullptr);
    }
    surface_items_.remove(toplevel->surface());
    states_.remove(toplevel);
  }

 protected:
  bool beforeHandleEvent(WSeat*, WSurface*, QObject*, QObject*,
      QInputEvent* event) override {
    if (!moving_toplevel_) {
      return false;
    }

    auto it = states_.find(moving_toplevel_);
    if (it == states_.end() || !it->item) {
      moving_toplevel_.clear();
      return false;
    }

    if (event->type() == QEvent::MouseMove) {
      const QPointF delta = compositor_->cursor_->position()
        - move_start_cursor_;
      it->item->setPosition(move_start_position_ + delta);
      compositor_->UpdateSurfaceOutputs(moving_toplevel_->surface());
      return true;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
      WXdgToplevelSurface* toplevel = moving_toplevel_;
      moving_toplevel_.clear();

      // Match Treeland's edge-tiling policy: reaching the top five pixels of
      // an output while moving arms maximize, and releasing applies it.
      const QPointF cursor = compositor_->cursor_->position();
      WOutput* output = OutputAt(cursor);
      if (output && cursor.y() <= output->position().y() + 5.0) {
        Maximize(toplevel, output);
      }
    }
    return false;
  }

  bool afterHandleEvent(WSeat* seat, WSurface* watched,
      QObject* shell_object, QObject*, QInputEvent* event) override {
    if (!event->isSinglePointEvent()
        || !static_cast<QSinglePointEvent*>(event)->isBeginEvent()) {
      return false;
    }

    auto* surface_item = qobject_cast<WSurfaceItem*>(shell_object);
    auto* toplevel = surface_item
      ? qobject_cast<WXdgToplevelSurface*>(surface_item->shellSurface())
      : nullptr;
    if (!toplevel || toplevel->surface() != watched) {
      return false;
    }

    Activate(seat, toplevel, surface_item);
    return false;
  }

  bool unacceptedEvent(WSeat* seat, QWindow*, QInputEvent* event) override {
    if (!event->isSinglePointEvent()
        || !static_cast<QSinglePointEvent*>(event)->isBeginEvent()) {
      return false;
    }

    Activate(seat, nullptr);
    return false;
  }

 private:
  struct ToplevelState {
    QPointer<WSurfaceItem> item;
    QPointF normal_position;
    QSize normal_size;
    QSizeF normal_item_size;
    bool maximized = false;
    bool minimized = false;
  };

  WOutput* OutputFor(WXdgToplevelSurface* toplevel) const {
    for (WOutput* output : toplevel->surface()->outputs()) {
      if (output->isEnabled()) {
        return output;
      }
    }

    const QList<WOutput*>& outputs = static_cast<WOutputLayout*>(
      compositor_->output_layout_)->outputs();
    for (WOutput* output : outputs) {
      if (output->isEnabled()) {
        return output;
      }
    }
    return nullptr;
  }

  WOutput* OutputAt(const QPointF& position) const {
    const QList<WOutput*>& outputs = static_cast<WOutputLayout*>(
      compositor_->output_layout_)->outputs();
    for (WOutput* output : outputs) {
      const QRectF geometry(QPointF(output->position()),
        QSizeF(output->effectiveSize()));
      if (output->isEnabled() && geometry.contains(position)) {
        return output;
      }
    }
    return nullptr;
  }

  void Maximize(WXdgToplevelSurface* toplevel,
      WOutput* target_output = nullptr) {
    auto it = states_.find(toplevel);
    WOutput* output = target_output ? target_output : OutputFor(toplevel);
    if (it == states_.end() || !it->item || !output || it->maximized
        || it->minimized || !toplevel->isInitialized()) {
      return;
    }

    it->normal_position = it->item->position();
    it->normal_size = toplevel->getContentGeometry().size();
    it->normal_item_size = it->item->size();
    if (it->normal_size.isEmpty()) {
      it->normal_size = it->item->size().toSize();
    }
    it->maximized = true;

    it->item->setPosition(output->position());
    toplevel->setMaximize(true);
    toplevel->resize(output->effectiveSize());
  }

  void Unmaximize(WXdgToplevelSurface* toplevel) {
    auto it = states_.find(toplevel);
    if (it == states_.end() || !it->item || !it->maximized
        || !toplevel->isInitialized()) {
      return;
    }

    it->maximized = false;
    it->item->setPosition(it->normal_position);
    toplevel->setMaximize(false);
    if (!it->normal_size.isEmpty()) {
      toplevel->resize(it->normal_size);
    }
  }

  void RestoreForMove(WXdgToplevelSurface* toplevel,
      ToplevelState& state) {
    const QPointF cursor = compositor_->cursor_->position();
    const QRectF maximized_geometry(state.item->position(),
      state.item->size());
    const QSizeF restored_size = state.normal_item_size.isEmpty()
      ? QSizeF(state.normal_size) : state.normal_item_size;

    const qreal fx = maximized_geometry.width() > 0
      ? (cursor.x() - maximized_geometry.left())
          / maximized_geometry.width()
      : 0.5;
    const qreal fy = maximized_geometry.height() > 0
      ? (cursor.y() - maximized_geometry.top())
          / maximized_geometry.height()
      : 0.0;
    const QPointF restored_position(
      cursor.x() - fx * restored_size.width(),
      cursor.y() - fy * restored_size.height());

    state.maximized = false;
    state.normal_position = restored_position;
    state.item->setPosition(restored_position);
    toplevel->setMaximize(false);
    if (!state.normal_size.isEmpty()) {
      toplevel->resize(state.normal_size);
    }
  }

  void Minimize(WXdgToplevelSurface* toplevel) {
    auto it = states_.find(toplevel);
    if (it == states_.end() || !it->item || it->minimized) {
      return;
    }

    it->minimized = true;
    it->item->setVisible(false);
    toplevel->setMinimize(true);
    if (active_toplevel_ == toplevel) {
      if (toplevel->isInitialized()) {
        toplevel->setActivate(false);
      }
      active_toplevel_.clear();
      compositor_->seat_->setKeyboardFocusSurface(nullptr);
    }
  }

  FlakeCompositor* compositor_ = nullptr;
  QHash<WXdgToplevelSurface*, ToplevelState> states_;
  QHash<WSurface*, QPointer<WSurfaceItem>> surface_items_;
  QPointer<WXdgToplevelSurface> active_toplevel_;
  QPointer<WXdgToplevelSurface> moving_toplevel_;
  QPointF move_start_cursor_;
  QPointF move_start_position_;
};

FlakeCompositor::FlakeCompositor(QObject* parent) : QObject(parent),
    server_(new WServer(this)),
    cursor_(new WCursor(this)),
    output_layout_(new WQuickOutputLayout(server_)),
    outputs_(new WQmlCreator(this)),
    toplevels_(new WQmlCreator(this)),
    popups_(new WQmlCreator(this)),
    seat_event_filter_(new SeatEventFilter(this)) {
  // Referred waylib/examples/tinywl/helper.cpp's Helper::init()
  // Both seat and backend are mounted as WServer interface, and this must be
  // done before WServer::start().
  seat_ = server_->attach<WSeat>();
  seat_->setEventFilter(seat_event_filter_);
  seat_->setCursor(cursor_);

  // cursor_ shares WQuickOutputLayout w/ OutputItem.
  // The global coordinate of cursor_ can be correctly converted to local
  // coordinates.
  // The layut object is parented to server_, so it will be destroyed later
  // than outputs.
  cursor_->setLayout(output_layout_);

  // Treeland's RootSurfaceContainer keeps every client surface's output set
  // in sync with the output layout. This is not only output metadata:
  // WSurface selects its frame-pacing output from this set. Without it,
  // clients receive no frame callbacks after their first buffer commit.
  connect(static_cast<WOutputLayout*>(output_layout_),
      &WOutputLayout::outputsChanged, this,
      [this]() { UpdateSurfaceOutputs(); });

  backend_ = server_->attach<WBackend>();

  // 5 is the xdg_wm_base that published to clients, following the current
  // Waylib TinyWL example.
  // TODO: This is only a minimal implementation, advanced features, such as
  //       layer-shell, shall be done later.
  xdg_shell_ = server_->attach<WXdgShell>(5);
}

WQmlCreator* FlakeCompositor::Outputs() const {
  return outputs_;
}

WQmlCreator* FlakeCompositor::Toplevels() const {
  return toplevels_;
}

WQmlCreator* FlakeCompositor::Popups() const {
  return popups_;
}

WQuickOutputLayout* FlakeCompositor::OutputLayout() const {
  return output_layout_;
}

WCursor* FlakeCompositor::Cursor() const {
  return cursor_;
}

void FlakeCompositor::RegisterSurface(WSurface* surface) {
  if (!surface || surfaces_.contains(surface)) {
    return;
  }

  surfaces_.append(surface);
  UpdateSurfaceOutputs(surface);
}

void FlakeCompositor::UnregisterSurface(WSurface* surface) {
  if (!surface || !surfaces_.removeOne(surface)) {
    return;
  }

  const QList<WOutput*> current_outputs = surface->outputs();
  for (WOutput* output : current_outputs) {
    surface->leaveOutput(output);
  }
}

void FlakeCompositor::UpdateSurfaceOutputs() {
  for (WSurface* surface : std::as_const(surfaces_)) {
    UpdateSurfaceOutputs(surface);
  }
}

void FlakeCompositor::UpdateSurfaceOutputs(WSurface* surface) {
  if (!surface) {
    return;
  }

  QList<WOutput*> target_outputs;
  const QList<WOutput*>& layout_outputs =
    static_cast<WOutputLayout*>(output_layout_)->outputs();
  for (WOutput* output : layout_outputs) {
    if (output->isEnabled()) {
      target_outputs.append(output);
    }
  }

  const QList<WOutput*> current_outputs = surface->outputs();
  for (WOutput* output : current_outputs) {
    if (!target_outputs.contains(output)) {
      surface->leaveOutput(output);
    }
  }
  for (WOutput* output : target_outputs) {
    if (!current_outputs.contains(output)) {
      surface->enterOutput(output);
    }
  }

  // A client may have committed its first frame while the output was still
  // being enabled. Now that framePacingOutput is available, wake that frame.
  surface->scheduleFrameIfNeeded();
}

QString FlakeCompositor::SocketName() const {
  return socket_ ? socket_->fullServerName() : QString();
}

bool FlakeCompositor::Start(WOutputRenderWindow* window, QQmlEngine* engine) {
  // Start() creates non-repeatable Wayland globals and a socket, so a failed
  // call must not be blindly retried on the same instance. The upper layer
  // should exit and reconstruct the entire compositor.
  if (started_ || !window || !engine) {
    return false;
  }
  started_ = true;

  // Like Treeland's RootSurfaceContainer/SeatsManager setup, route pointer
  // and keyboard events from wlroots into the Qt Quick render window. Without
  // an event window pointer motion may still update the cursor position, but
  // button events have no QWindow target and never reach WSurfaceItem.
  cursor_->setEventWindow(window);
  seat_->setKeyboardFocusWindow(window);

  // The output/input signal wiring references
  // outputviewport/helper.cpp::initProtocols() and tinywl/helper.cpp::init().
  // Unlike the examples, FlakeWM hands objects to QML via WQmlCreator instead
  // of directly `new OutputItem` in C++.
  connect(backend_, &WBackend::outputAdded, this,
      [this, engine](WOutput* output) {
    // Nested Wayland/X11 backends typically lack a DRM hardware cursor
    // plane. Forcing a software cursor ensures the cursor still appears
    // in the composited result under development environments.
    if (!backend_->hasDrm()) {
      output->setForceSoftwareCursor(true);
    }

    connect(output, &WOutput::enabledChanged, this,
        [this]() { UpdateSurfaceOutputs(); });

    // waylandOutput maps to the required property in Main.qml.
    // x is set to the current layout width, implementing FlakeWM's
    // temporary "new outputs stack left to right" strategy. This is
    // not derived from Treeland or TinyWL.
    QJSValue properties = engine->newObject();
    properties.setProperty(QStringLiteral("waylandOutput"),
      engine->toScriptValue(output));
    properties.setProperty(QStringLiteral("x"),
      output_layout_->implicitWidth());
    outputs_->add(output, properties);
  });

  connect(backend_, &WBackend::outputRemoved, this,
      [this](WOutput* output) {
    outputs_->removeByOwner(output);
  });

  connect(backend_, &WBackend::inputAdded, this,
      [this](WInputDevice* device) {
    seat_->attachInputDevice(device);
  });

  connect(backend_, &WBackend::inputRemoved, this,
      [this](WInputDevice* device) {
    seat_->detachInputDevice(device);
  });

  connect(toplevels_, &WQmlCreator::objectAdded, this,
      [this](WAbstractCreatorComponent*, QObject* object,
          const QJSValue& properties) {
    auto* item = qobject_cast<WXdgToplevelSurfaceItem*>(object);
    auto* surface = qobject_cast<WXdgToplevelSurface*>(
      properties.property(QStringLiteral("waylandSurface")).toQObject());
    if (item && surface) {
      seat_event_filter_->BindItem(surface, item);
    }
  });

  connect(popups_, &WQmlCreator::objectAdded, this,
      [this](WAbstractCreatorComponent*, QObject* object,
          const QJSValue& properties) {
    auto* item = qobject_cast<WXdgPopupSurfaceItem*>(object);
    auto* surface = qobject_cast<WXdgPopupSurface*>(
      properties.property(QStringLiteral("waylandSurface")).toQObject());
    if (item && surface) {
      seat_event_filter_->BindPopupItem(surface, item);
    }
  });

  // xdg-shell routing references tinywl/helper.cpp.
  // Once the owner is given to WQmlCreator, removeByOwner synchronously
  // removes the QML delegate when the protocol object is destroyed, avoiding
  // dangling shellSurface references.
  connect(xdg_shell_, &WXdgShell::toplevelSurfaceAdded, this,
      [this, engine](WXdgToplevelSurface* surface) {
    QJSValue properties = engine->newObject();
    properties.setProperty(QStringLiteral("waylandSurface"),
      engine->toScriptValue(surface));
    seat_event_filter_->Register(surface);
    toplevels_->add(surface, properties);
    RegisterSurface(surface->surface());

    // An xdg-toplevel is announced before its role has completed the initial
    // configure handshake. Calling setActivate() here trips wlroots'
    // `surface->initialized` assertion. Treeland likewise gates activation on
    // MappedOrSplash; activate only after the backing wl_surface is mapped.
    WSurface* wayland_surface = surface->surface();
    connect(wayland_surface, &WSurface::mappedChanged, this,
        [this, surface, wayland_surface]() {
      if (wayland_surface->mapped() && surface->isInitialized()) {
        seat_event_filter_->Activate(seat_, surface);
      }
    });
  });

  connect(xdg_shell_, &WXdgShell::toplevelSurfaceRemoved, this,
      [this](WXdgToplevelSurface* surface) {
    seat_event_filter_->Remove(seat_, surface);
    UnregisterSurface(surface->surface());
    toplevels_->removeByOwner(surface);
  });

  connect(xdg_shell_, &WXdgShell::popupSurfaceAdded, this,
      [this, engine](WXdgPopupSurface* surface) {
    QJSValue properties = engine->newObject();
    properties.setProperty(QStringLiteral("waylandSurface"),
      engine->toScriptValue(surface));
    popups_->add(surface, properties);
    RegisterSurface(surface->surface());
  });

  connect(xdg_shell_, &WXdgShell::popupSurfaceRemoved, this,
      [this](WXdgPopupSurface* surface) {
    seat_event_filter_->RemovePopup(surface);
    UnregisterSurface(surface->surface());
    popups_->removeByOwner(surface);
  });

  // WServer::start() first creates the wl_display and globals for all attached
  // interfaces. The startup order of renderer, socket and backend follows the
  // outputviewport and TinyWL examples.
  server_->start();

  // WRenderHelper selects a renderer based on WLR_RENDERER and backend
  // capabilities. The allocator must use the same backend/renderer pair,
  // otherwise client buffers may fail to import.
  renderer_ = WRenderHelper::createRenderer(backend_->handle());
  if (!renderer_) {
    qCritical() << "(Compositor) Init: Failed to create a renderer!";
    return false;
  }

  allocator_ = wlr_allocator_autocreate(backend_->handle(), renderer_);
  if (!allocator_) {
    qCritical() << "(Compositor) Init: Failed to create a buffer allocator!";
    return false;
  }

  // Register the renderer's shm/dmabuf capabilities with wl_display, then
  // publish wl_compositor v6 and wl_subcompositor. This initialisation
  // sequence is taken directly from outputviewport/helper.cpp.
  wlr_renderer_init_wl_display(renderer_, server_->handle());
  compositor_ = wlr_compositor_create(server_->handle(), 6, renderer_);
  wlr_subcompositor_create(server_->handle());

  // Reference outputviewport/helper.cpp: wait until OutputViewport has
  // created its swapchain and render target before enabling the physical
  // output. This avoids the backend sending frames while the Qt scene has no
  // render target yet.
  connect(window, &WOutputRenderWindow::outputViewportInitialized, this,
      [](WOutputViewport* viewport) {
    WOutput* output = viewport->output();
    if (output->property("_flakewmEnabled").toBool()) {
      return;
    }

    wlr_output* handle = output->handle();
    wlr_output_state state;
    wlr_output_state_init(&state);

    // Nested outputs often already have a current_mode; real DRM
    // outputs prefer the preferred mode on first enable.
    // _flakewmEnabled is a local idempotency guard to avoid repeated
    // commits.
    if (!handle->current_mode) {
      if (auto* mode = wlr_output_preferred_mode(handle)) {
        wlr_output_state_set_mode(&state, mode);
      }
    }

    wlr_output_state_set_enabled(&state, true);
    if (wlr_output_commit_state(handle, &state)) {
      output->setProperty("_flakewmEnabled", true);
    } else {
      qCritical() << "(Compositor) Init: Failed to enable output" << output;
    }
    wlr_output_state_finish(&state);
  });

  window->init(renderer_, allocator_);

  // Reference tinywl/helper.cpp: `false` means we do not take over the socket
  // as an external fd. autoCreate() picks an available wayland-N under
  // XDG_RUNTIME_DIR, then hands it to WServer to listen on the event loop.
  socket_ = new WSocket(false, server_);
  if (!socket_->autoCreate()) {
    qCritical() << "(Compositor) Init: Failed to create a Wayland socket!!";
    return false;
  }
  server_->addSocket(socket_);
  Q_EMIT socketNameChanged();

  // The backend is started last. Once started it immediately enumerates
  // outputs/inputs and emits the signals wired above, so it must not be
  // brought forward before creator, renderer, window or socket initialisation.
  if (!wlr_backend_start(backend_->handle())) {
    qCritical() << "(Compositor) Init: Failed to start the wlroots backend!!";
    return false;
  }

  qInfo().noquote()
    << "(Compositor) Init: FlakeWM is now listening on"
    << SocketName();

  return true;
}
