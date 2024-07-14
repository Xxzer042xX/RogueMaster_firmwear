#pragma once

#define ISO7816_RESPONSE_OK 0x9000

#define ISO7816_RESPONSE_WRONG_LENGTH 0x6700
#define ISO7816_RESPONSE_WRONG_PARAMETERS_P1_P2 0x6A00
#define ISO7816_RESPONSE_WRONG_LE 0x6C00
#define ISO7816_RESPONSE_INSTRUCTION_NOT_SUPPORTED 0x6D00
#define ISO7816_RESPONSE_CLASS_NOT_SUPPORTED 0x6E00
#define ISO7816_RESPONSE_INTERNAL_EXCEPTION 0x6F00

void iso7816_set_response(ISO7816_Response_APDU* responseAPDU, uint16_t responseCode);