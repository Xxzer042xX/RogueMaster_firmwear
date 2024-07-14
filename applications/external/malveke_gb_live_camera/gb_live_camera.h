// TODO
// (DONE) Fix performance when not being charged
// (DONE) Add UART command parsing to Esp32
// (DONE) Prepare application and icon on github
// (DONE) Write snapshots to SD card
// 5. Set a constant refresh rate to the Flipper's display
// 6. Emulate grayscale
// 7. Photo browser app

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/icon_i.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <gui/elements.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/dialog_ex.h>
#include <storage/filesystem_api_defines.h>
#include <storage/storage.h>
#include <expansion/expansion.h>

#define THREAD_ALLOC 2048
#define BAUDRATE (115200UL)

#define FRAME_WIDTH 128
#define FRAME_HEIGTH 64
#define FRAME_BIT_DEPTH 1
#define FRAME_BUFFER_LENGTH \
    (FRAME_WIDTH * FRAME_HEIGTH * FRAME_BIT_DEPTH / 8) // 128*64*1 / 8 = 1024
#define ROW_BUFFER_LENGTH (FRAME_WIDTH / 8) // 128/8 = 16
#define LAST_ROW_INDEX (FRAME_BUFFER_LENGTH - ROW_BUFFER_LENGTH) // 1024 - 16 = 1008
#define RING_BUFFER_LENGTH (ROW_BUFFER_LENGTH + 3) // ROW_BUFFER_LENGTH + Header => 16 + 3 = 19
#define BITMAP_HEADER_LENGTH 62

#define MALVEKE_APP_FOLDER_USER "apps_data/malveke"
#define MALVEKE_APP_FOLDER EXT_PATH(MALVEKE_APP_FOLDER_USER)
#define MALVEKE_APP_FOLDER_PHOTOS MALVEKE_APP_FOLDER "/photos"

// #define IMAGE_FILE_DIRECTORY_PATH EXT_PATH("DCIM")

static const unsigned char bitmap_header[BITMAP_HEADER_LENGTH] = {
    0x42, 0x4D, 0x3E, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x00};
static const unsigned char bitmap_header_gameboy_full[BITMAP_HEADER_LENGTH] = {
    0x42, 0x4D, 0x80, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x90, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x42, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t _I_DolphinCommon_56x48_0[] = {
    0x01, 0x00, 0xdf, 0x00, 0x00, 0x1f, 0xfe, 0x0e, 0x05, 0x3f, 0x04, 0x06, 0x78, 0x06, 0x30, 0x20,
    0xf8, 0x00, 0xc6, 0x12, 0x1c, 0x04, 0x0c, 0x0a, 0x38, 0x08, 0x08, 0x0c, 0x60, 0xc0, 0x21, 0xe0,
    0x04, 0x0a, 0x18, 0x02, 0x1b, 0x00, 0x18, 0xa3, 0x00, 0x21, 0x90, 0x01, 0x8a, 0x20, 0x02, 0x19,
    0x80, 0x18, 0x80, 0x64, 0x09, 0x20, 0x89, 0x81, 0x8c, 0x3e, 0x41, 0xe2, 0x80, 0x50, 0x00, 0x43,
    0x08, 0x01, 0x0c, 0xfc, 0x68, 0x40, 0x61, 0xc0, 0x50, 0x30, 0x00, 0x63, 0xa0, 0x7f, 0x80, 0xc4,
    0x41, 0x19, 0x07, 0xff, 0x02, 0x06, 0x18, 0x24, 0x03, 0x41, 0xf3, 0x2b, 0x10, 0x19, 0x38, 0x10,
    0x30, 0x31, 0x7f, 0xe0, 0x34, 0x08, 0x30, 0x19, 0x60, 0x80, 0x65, 0x86, 0x0a, 0x4c, 0x0c, 0x30,
    0x81, 0xb9, 0x41, 0xa0, 0x54, 0x08, 0xc7, 0xe2, 0x06, 0x8a, 0x18, 0x25, 0x02, 0x21, 0x0f, 0x19,
    0x88, 0xd8, 0x6e, 0x1b, 0x01, 0xd1, 0x1b, 0x86, 0x39, 0x66, 0x3a, 0xa4, 0x1a, 0x50, 0x06, 0x48,
    0x18, 0x18, 0xd0, 0x03, 0x01, 0x41, 0x98, 0xcc, 0x60, 0x39, 0x01, 0x49, 0x2d, 0x06, 0x03, 0x50,
    0xf8, 0x40, 0x3e, 0x02, 0xc1, 0x82, 0x86, 0xc7, 0xfe, 0x0f, 0x28, 0x2c, 0x91, 0xd2, 0x90, 0x9a,
    0x18, 0x19, 0x3e, 0x6d, 0x73, 0x12, 0x16, 0x00, 0x32, 0x49, 0x72, 0xc0, 0x7e, 0x5d, 0x44, 0xba,
    0x2c, 0x08, 0xa4, 0xc8, 0x82, 0x06, 0x17, 0xe0, 0x81, 0x90, 0x2a, 0x40, 0x61, 0xe1, 0xa2, 0x44,
    0x0c, 0x76, 0x2b, 0xe8, 0x89, 0x26, 0x43, 0x83, 0x31, 0x8c, 0x78, 0x0c, 0xb0, 0x48, 0x10, 0x1a,
    0xe0, 0x00, 0x63,
};
const uint8_t* const _I_DolphinCommon_56x48[] = {_I_DolphinCommon_56x48_0};
const Icon I_DolphinCommon_56x48 = {
    .width = 56,
    .height = 48,
    .frame_count = 1,
    .frame_rate = 0,
    .frames = _I_DolphinCommon_56x48};

typedef struct UartDumpModel UartDumpModel;

typedef struct {
    Gui* gui;
    Storage* storage;
    NotificationApp* notification;
    ViewDispatcher* view_dispatcher;
    View* view;
    FuriThread* worker_thread;
    FuriStreamBuffer* rx_stream;
    FuriHalSerialHandle* serial_handle_uart;
    FuriHalSerialHandle* serial_handle_lp_uart;
} UartEchoApp;

struct UartDumpModel {
    uint8_t pixels[FRAME_BUFFER_LENGTH];

    File* cameraSav;
    bool initialized;
    int page;
    uint8_t row_ringbuffer[RING_BUFFER_LENGTH];
    uint8_t ringbuffer_index;
};

typedef enum {
    WorkerEventReserved = (1 << 0), // Reserved for StreamBuffer internal event
    WorkerEventStop = (1 << 1),
    WorkerEventRx = (1 << 2),
} WorkerEventFlags;

#define WORKER_EVENTS_MASK (WorkerEventStop | WorkerEventRx)

const NotificationSequence sequence_notification = {
    // &message_display_backlight_on,
    &message_delay_10,
    NULL,
};