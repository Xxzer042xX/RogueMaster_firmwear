#include <furi.h>

#include "quac.h"
#include "quac_settings.h"

#include "item.h"
#include "scenes/scenes.h"
#include "scenes/scene_items.h"

/* generated by fbt from .png files in images folder */
#include <quac_icons.h>

App* app_alloc() {
    App* app = malloc(sizeof(App));
    app->scene_manager = scene_manager_alloc(&app_scene_handlers, app);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, app_scene_custom_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, app_back_event_callback);

    // Create our UI elements
    // Main interface
    app->action_menu = action_menu_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, QView_ActionMenu, action_menu_get_view(app->action_menu));

    // App settings
    app->vil_settings = variable_item_list_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, QView_Settings, variable_item_list_get_view(app->vil_settings));

    // Misc interfaces
    app->sub_menu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, QView_SubMenu, submenu_get_view(app->sub_menu));

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, QView_TextInput, text_input_get_view(app->text_input));

    app->popup = popup_alloc();
    view_dispatcher_add_view(app->view_dispatcher, QView_Popup, popup_get_view(app->popup));

    // Storage
    app->storage = furi_record_open(RECORD_STORAGE);

    // Notifications - for LED light access
    app->notifications = furi_record_open(RECORD_NOTIFICATION);

    app->dialog = furi_record_open(RECORD_DIALOGS);

    // data member initialize
    app->depth = 0;
    app->selected_item = -1;

    app->temp_str = furi_string_alloc();

    return app;
}

void app_free(App* app) {
    furi_assert(app);

    item_items_view_free(app->items_view);

    view_dispatcher_remove_view(app->view_dispatcher, QView_ActionMenu);
    view_dispatcher_remove_view(app->view_dispatcher, QView_Settings);
    view_dispatcher_remove_view(app->view_dispatcher, QView_SubMenu);
    view_dispatcher_remove_view(app->view_dispatcher, QView_TextInput);
    view_dispatcher_remove_view(app->view_dispatcher, QView_Popup);

    action_menu_free(app->action_menu);
    variable_item_list_free(app->vil_settings);
    submenu_free(app->sub_menu);
    text_input_free(app->text_input);

    scene_manager_free(app->scene_manager);
    view_dispatcher_free(app->view_dispatcher);

    furi_string_free(app->temp_str);

    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_DIALOGS);

    free(app);
}

// FAP Entry Point
int32_t quac_app(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "QUAC! QUAC!");

    size_t free_start = memmgr_get_free_heap();
    furi_assert(0);

    App* app = app_alloc();
    quac_load_settings(app);

    // Read items at our root
    app->items_view = item_get_items_view_from_path(app, NULL);

    Gui* gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    scene_manager_next_scene(app->scene_manager, QScene_Items);
    view_dispatcher_run(app->view_dispatcher);

    furi_record_close(RECORD_GUI);
    app_free(app);

    size_t free_end = memmgr_get_free_heap();
    FURI_LOG_W(TAG, "Heap: Start = %d, End = %d", free_start, free_end);

    return 0;
}
