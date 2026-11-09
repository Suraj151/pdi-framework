/*********** SMTP (Simple Mail Transfer Protocol) Client **********************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _SMTP_DRIVER_H_
#define _SMTP_DRIVER_H_

#include <config/Config.h>
#include <utility/Utility.h>
#include <utility/Base64.h>
#include <helpers/ClientHelper.h>

/*
 *  reference  https://tools.ietf.org/html/rfc821#page-36
 *
 *  CONNECTION ESTABLISHMENT
                S: 220
                F: 421
              HELO
                S: 250
                E: 500, 501, 504, 421
              MAIL
                S: 250
                F: 552, 451, 452
                E: 500, 501, 421
              RCPT
                 S: 250, 251
                 F: 550, 551, 552, 553, 450, 451, 452
                 E: 500, 501, 503, 421
              DATA
                 I: 354 -> data -> S: 250
                                   F: 552, 554, 451, 452
                 F: 451, 554
                 E: 500, 501, 503, 421
              RSET
                 S: 250
                 E: 500, 501, 504, 421
              SEND
                 S: 250
                 F: 552, 451, 452
                 E: 500, 501, 502, 421
              SOML
                 S: 250
                 F: 552, 451, 452
                 E: 500, 501, 502, 421
              SAML
                 S: 250
                 F: 552, 451, 452
                 E: 500, 501, 502, 421
              VRFY
                 S: 250, 251
                 F: 550, 551, 553
                 E: 500, 501, 502, 504, 421
              EXPN
                 S: 250
                 F: 550
                 E: 500, 501, 502, 504, 421
              HELP
                 S: 211, 214
                 E: 500, 501, 502, 504, 421
              NOOP
                 S: 250
                 E: 500, 421
              QUIT
                 S: 221
                 E: 500
              TURN
                 S: 250
                 F: 502
                 E: 500, 503

 *  For each command there are three possible outcomes:  "success" (S), "failure" (F), and "error" (E).
*/

#define SMTP_COMMAND_EHLO "EHLO"
#define SMTP_COMMAND_AUTH "AUTH LOGIN"
#define SMTP_COMMAND_FROM "MAIL FROM"
#define SMTP_COMMAND_TO "RCPT TO"
#define SMTP_COMMAND_DATA "DATA"
#define SMTP_COMMAND_DATA_HEADER_FROM "From: "
#define SMTP_COMMAND_DATA_HEADER_TO "To: "
#define SMTP_COMMAND_DATA_HEADER_SUBJECT "Subject: "
#define SMTP_COMMAND_DATA_TERMINATOR "."
#define SMTP_COMMAND_QUIT "QUIT"
#define SMTP_COMMAND_SEPARATOR ":"
#define SMTP_COMMAND_OPENING_ANG_BRACKET "<"
#define SMTP_COMMAND_CLOSING_ANG_BRACKET ">"
#define SMTP_COMMAND_CRLF "\r\n"

enum smtp_reply_code
{
   SMTP_STATUS_SYSTEM_STATUS = 211,
   SMTP_STATUS_HELP_MESSAGE = 214,
   SMTP_STATUS_SERVER_READY = 220,
   SMTP_STATUS_SERVER_ENDING_CON = 221,
   SMTP_STATUS_AUTH_SUCCEEDED = 235,
   SMTP_STATUS_ACTION_COMPLETED = 250,
   SMTP_STATUS_USER_NOT_LOCAL_WILL_FORWARD = 251,
   SMTP_STATUS_AUTH_CONTINUE = 334,
   SMTP_STATUS_START_MAIL_INPUT = 354,

   SMTP_STATUS_SERVICE_UNAVAILABLE = 421,
   SMTP_STATUS_MAILBOX_UNAVAILABLE = 450,
   SMTP_STATUS_LOCAL_ERROR_IN_PROCESS = 451,
   SMTP_STATUS_INSUFFICIENT_SYSTEM_STORAGE = 452,

   SMTP_STATUS_SYNTAX_ERROR_COMMAND = 500,
   SMTP_STATUS_SYNTAX_ERROR_ARGUMENT = 501,
   SMTP_STATUS_COMMAND_NOT_IMPLEMENTED = 502,
   SMTP_STATUS_COMMAND_BAD_SEQUENCES = 503,
   SMTP_STATUS_PARMETERS_NOT_IMPLEMENTED = 504,
   SMTP_STATUS_MAILBOX_UNAVAILABLE_NO_ACCESS = 550,
   SMTP_STATUS_USER_NOT_LOCAL = 551,
   SMTP_STATUS_EXCEED_STORAGE_ALLOC = 552,
   SMTP_STATUS_MAILBOX_NAME_NOT_ALLOWED = 553,
   SMTP_STATUS_TRANSACTION_FAILED = 554,
   SMTP_STATUS_MAX = 999
};

enum smtp_command_status : uint8_t
{
   SMTP_RESPONSE_CONN_ERR,
   SMTP_RESPONSE_STARTING,
   SMTP_RESPONSE_WAITING,
   SMTP_RESPONSE_TIMEOUT,
   SMTP_RESPONSE_BUFFER_FULL,
   SMTP_RESPONSE_FINISHED,
   SMTP_RESPONSE_ERROR,
   SMTP_RESPONSE_MAX
};

#define SMTP_RESPONSE_BUFFER_SIZE 520
#define SMTP_DEFAULT_TIMEOUT MILLISECOND_DURATION_5000

/**
 * SMTPClient class
 */
class SMTPClient
{

public:
   SMTPClient();
   ~SMTPClient();

   /**
    * Connects to the server and starts reading its greeting. False when the
    * connection could not be made.
    */
   bool begin(iClientInterface *_client, char *_host, uint16_t _port, bool _isSecure = false);

   /**
    * Reads what the server has sent of the reply being waited for, without
    * waiting for more. The reply code is kept once the reply is complete.
    */
   smtp_command_status pollResponse(void);

   /**
    * Sends EHLO with the domain the board greets as, and starts reading its reply.
    */
   void startHello(char *domain);

   /**
    * Sends AUTH LOGIN, and starts reading the reply that asks for the username.
    */
   void startAuthLogin(void);

   /**
    * Sends the username of an AUTH LOGIN, and starts reading its reply.
    */
   void startAuthUser(char *username);

   /**
    * Sends the password of an AUTH LOGIN, and starts reading its reply.
    */
   void startAuthPass(char *password);

   /**
    * Sends MAIL FROM with the sender address, and starts reading its reply.
    */
   void startFrom(char *sender);

   /**
    * Sends RCPT TO with the recipient address, and starts reading its reply.
    */
   void startTo(char *recipient);

   /**
    * Sends DATA, and starts reading the reply that asks for the message.
    */
   void startDataCommand(void);

   /**
    * Writes the message headers and body, then the line that ends the message.
    */
   void startDataBody(char *sender, char *recipient, char *subject, const char *body, uint32_t size);

   /**
    * Sends QUIT, and starts reading its reply.
    */
   void startQuit(void);
   void end(void);

   /**
    * the code of the server's last reply, 0 when none was read
    */
   int16_t lastResponseCode(void) const { return m_lastResponseCode; }

protected:
   /**
    * writes a message body with each line ended in CRLF and a leading dot doubled
    */
   void writeBody(const char *body, uint32_t size);

   /**
    * Sends one command line, dropping what is left of an earlier reply, and
    * starts reading its reply.
    */
   void startCommand(const char *command);

   /**
    * Writes a command made of a verb and an address in angle brackets.
    */
   void startAddressCommand(const char *verb, char *address);

   /**
    * Writes a credential encoded in base64 as the next line of a login.
    */
   void startEncoded(char *text);

   /**
    * Reads what has arrived of the reply. A reply ends with the line whose code
    * is not followed by a dash; only its first SMTP_RESPONSE_BUFFER_SIZE bytes
    * are kept, which is where the code is. A connection the server closed ends
    * the wait at once.
    */
   void readResponse(void);
   void flushClient(void);
   void startReadResponse(uint16_t _timeOut = SMTP_DEFAULT_TIMEOUT);

   iClientInterface *m_client;
   uint16_t m_port;
   int16_t m_lastResponseCode;
   uint16_t m_responseBufferIndex;
   uint16_t m_lineLen;
   uint32_t m_lastReceive;
   uint32_t m_timeOut;
   smtp_command_status m_responseReaderStatus;
   char *m_responseBuffer;
   char *m_host;
   char m_crlfBuf[2];
   char m_lineHead[4];
   bool m_isSecure;
};

#endif
