/******************************** Command Base ********************************
This file is part of the PDI stack.

This is free software. You can redistribute it and/or modify it but without any
warranty.

The CommandBase utility provides an abstraction layer for handling commands
within the PDI stack. It allows for defining commands, parsing arguments,
validating options, and executing commands with user-defined logic.

Author          : Suraj I.
Created Date    : 1st June 2019
******************************************************************************/

#ifndef _COMMAND_BASE_H_
#define _COMMAND_BASE_H_

#include "StringOperations.h"
#include "iIOInterface.h"

/**
 * @enum cmd_result_t
 * @brief Represents the result of a command execution.
 */
typedef enum {
    CMD_RESULT_OK = 0,               ///< Command executed successfully.
    CMD_RESULT_ARGS_ERROR,           ///< Argument error in the command.
    CMD_RESULT_ARGS_MISSING,         ///< Missing arguments for the command.
    CMD_RESULT_NOT_FOUND,            ///< Command not found.
    CMD_RESULT_INVALID,              ///< Invalid command.
    CMD_RESULT_INVALID_OPTION,       ///< Invalid option provided.
    CMD_RESULT_NEED_AUTH,            ///< Command requires authentication.
    CMD_RESULT_INCOMPLETE,           ///< Command execution is incomplete.
    CMD_RESULT_WRONG_CREDENTIAL,     ///< Wrong credentials provided.
    CMD_RESULT_FAILED,               ///< Failed
    CMD_RESULT_MAX                   ///< Unknown or unhandled result.
} cmd_result_t;

/**
 * @enum cmd_status_t
 * @brief Represents the status of a command.
 */
typedef enum {
    CMD_STATUS_IDLE = 0,             ///< Command is idle.
    CMD_STATUS_ACTIVE,               ///< Command is active.
    CMD_STATUS_INACTIVE,             ///< Command is inactive.
    CMD_STATUS_MAX                   ///< Unknown or unhandled status.
} cmd_status_t;

/* Command constants */
#define CMD_SIZE_MAX                8   ///< Maximum size of a command.
#define CMD_OPTION_MAX              3   ///< Maximum number of options for a command.
#define CMD_OPTION_SIZE_MAX         3   ///< Maximum size of an option.
#define CMD_OPTION_SEPERATOR_COMMA  "," ///< Comma as a Separator for options.
#define CMD_OPTION_SEPERATOR_SPACE  " " ///< Space as a Separator for options.
#define CMD_OPTION_ASSIGN_OPERATOR  "=" ///< Assignment operator for options.

/**
 * @struct CommandBase
 * @brief Represents the base structure for a command.
 *
 * This structure provides the foundation for defining and executing commands.
 * It includes support for options, argument parsing, and terminal interaction.
 */
typedef struct CommandBase {

    /**
     * @struct CommandOption
     * @brief Represents an option for a command.
     *
     * This structure defines an individual option for a command, including its
     * name, value, and size.
     */
    struct CommandOption {
        char option[CMD_OPTION_SIZE_MAX]; ///< Name of the option.
        char *optionval;                 ///< Value of the option.
        int16_t optionvalsize;           ///< Size of the option value.
        bool holdingoptn;                ///< Indicates if the option value is held.

        /**
         * @brief Constructor for the CommandOption structure.
         *
         * Initializes the option with default values.
         */
        CommandOption() : optionval(nullptr), optionvalsize(0), holdingoptn(false) {
            Clear(true);
        }

        /**
         * @brief Clears the option data.
         * @param deep If true, clears the option name as well.
         */
        void Clear(bool deep = false){
            if(holdingoptn && nullptr != optionval && optionvalsize){
                delete[] optionval;
            }
            optionval = nullptr;
            optionvalsize = 0;
            holdingoptn = false;
            if(deep){
                memset(option, 0, CMD_OPTION_SIZE_MAX);
            }
        }
    };

    /* Members */
    char m_cmd[CMD_SIZE_MAX];                     ///< Command name.
    CommandOption m_options[CMD_OPTION_MAX];      ///< Array of command options.
    uint8_t m_optionindx;                         ///< Index of the current option.
    int8_t m_waitingoptionindx;                   ///< Index of the option waiting for input.
    iTerminalInterface *m_terminal;              ///< Terminal interface for command interaction.
    cmd_status_t m_status;                        ///< Status of the command.
    cmd_result_t m_result;                        ///< Result of the command execution.
    bool m_acceptArgsOptions;                   ///< Flag to accept argumental options.
    const char* m_optionseparator;               ///< Separator for options.

    /**
     * @brief Constructor for the CommandBase structure.
     *
     * Initializes the command with default values.
     */
    CommandBase(){
        // Clear();
    }

    /**
     * @brief Sets the terminal interface for the command.
     * @param terminal Pointer to the terminal interface.
     */
    void SetTerminal(iTerminalInterface *terminal){
        m_terminal = terminal;
    }

    /**
     * @brief Sets the command name.
     * @param _cmd The command name to set.
     * @return True if the command name was set successfully, false otherwise.
     */
    bool SetCommand(const char *_cmd){
        if(nullptr != _cmd){
            int16_t cmdsize = strlen(_cmd);
            if( cmdsize < CMD_SIZE_MAX ){
                memcpy(m_cmd, _cmd, cmdsize);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Adds an available option for the command.
     * @param _optn The option name to add.
     * @return True if the option was added successfully, false otherwise.
     */
    bool AddOption(const char *_optn){
        if(nullptr != _optn){
            int16_t optnsize = strlen(_optn);
            if( m_optionindx < CMD_OPTION_MAX && optnsize < CMD_OPTION_SIZE_MAX ){
                memcpy(m_options[m_optionindx].option, _optn, optnsize);
                m_optionindx++;
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Retrieves an available option from parsed command line data.
     * @param _optn The option name to retrieve.
     * @return Pointer to the CommandOption structure, or nullptr if not found.
     */
    CommandOption *RetrieveOption(const char *_optn){
        if( nullptr != _optn ){
            for (uint8_t i = 0; i < CMD_OPTION_MAX; i++){
                if (m_options[i].optionval != nullptr && m_options[i].optionvalsize > 0 &&
                    __are_str_equals(m_options[i].option, _optn, CMD_OPTION_SIZE_MAX)){
                    return &m_options[i];
                }
            }
        }
        return nullptr;
    }

    /**
     * @brief Checks if the passed argument matches the current command.
     * @param _cmd The command to validate.
     * @return True if the command matches, false otherwise.
     */
    bool isValidCommand(char *_cmd){

        if( nullptr == _cmd ){
            return false;
        }

        int16_t cmd_max_len = pdistd::min((size_t)CMD_SIZE_MAX, strlen(_cmd));
        int16_t cmd_start_indx = __strstr(_cmd, m_cmd, cmd_max_len);
        int16_t cmd_end_indx = __strstr(_cmd+cmd_start_indx, " ");
        cmd_end_indx = cmd_end_indx < 0 ? cmd_max_len : (cmd_start_indx+cmd_end_indx);
        // cmd_end_indx = cmd_end_indx > cmd_max_len ? cmd_max_len : cmd_end_indx;

        if( cmd_start_indx >= 0 && cmd_start_indx < cmd_end_indx && cmd_end_indx <= cmd_max_len ){

            char argcmd[CMD_SIZE_MAX];
            memset(argcmd, 0, CMD_SIZE_MAX);
            memcpy(argcmd, _cmd + cmd_start_indx, pdistd::min(CMD_SIZE_MAX, abs(cmd_end_indx - cmd_start_indx)));
    
            return __are_str_equals(m_cmd, argcmd, CMD_SIZE_MAX);
        }
        return false;
        // return ((nullptr != _cmd) && __are_arrays_equal(m_cmd, _cmd, strlen(m_cmd)));
    }

    /**
     * @brief Checks if the passed argument matches an available option and returns its index.
     * @param _optn The option name to validate.
     * @return The index of the option if valid, or -1 if invalid.
     */
    int8_t getOptionIndex(const char *_optn){
        for (uint8_t i = 0; nullptr != _optn && i < CMD_OPTION_MAX; i++){
            if(__are_str_equals(m_options[i].option, _optn, CMD_OPTION_SIZE_MAX)){
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief Checks if the passed argument is a valid option.
     * @param _optn The option name to validate.
     * @return True if the option is valid, false otherwise.
     */
    bool isValidOption(const char *_optn){
        return (getOptionIndex(_optn) != -1);
    }

    /**
     * @brief Checks if the command is waiting for an option.
     * @return True if the command is waiting for an option, false otherwise.
     */
    bool isWaitingForOption(){
        return (m_waitingoptionindx != -1);
    }

    /**
     * @brief Sets the flag indicating that the command needs more input from the user.
     * @param _optn The option name for which input is required.
     */
    void setWaitingForOption(const char *_optn){
        m_waitingoptionindx = getOptionIndex(_optn);
    }

    /**
     * @brief Sets whether to accept argument options.
     * @param _accept Indicates whether to accept argument options.
     */
    void setAcceptArgsOptions(bool _accept){
        m_acceptArgsOptions = _accept;
    }

    /**
     * @brief Sets the command option separator.
     * @param separator The separator string for options.
     */
    void setCmdOptionSeparator(const char* separator){
        m_optionseparator = separator;
    }

    /**
     * @brief Holds the value of an option if provided.
     * @param _optn The option name to hold the value for.
     * @return True if the option value was held successfully, false otherwise.
     */
    bool holdOptionValue(const char *_optn){
        bool bStatus = false;
        int8_t optindx = getOptionIndex(_optn);
        if( optindx != -1 && optindx < m_optionindx ){
            if( !m_options[optindx].holdingoptn &&
                nullptr != m_options[optindx].optionval && 
                m_options[optindx].optionvalsize ){
                char *val = new char[m_options[optindx].optionvalsize+2]();
                memset(val, 0, m_options[optindx].optionvalsize+2);
                memcpy(val, m_options[optindx].optionval, m_options[optindx].optionvalsize);
                m_options[optindx].optionval = val;
                m_options[optindx].holdingoptn = true;
            }
            bStatus = m_options[optindx].holdingoptn;
        }
        return bStatus;
    }

    /**
     * @brief Executes the command with the provided arguments.
     * @param _args The command arguments.
     * @param _len The length of the arguments.
     * @param _waiting_option Indicates if the command is waiting for an option.
     * @return The result of the command execution.
     */
    cmd_result_t executeCommand(char *_args, int16_t _len, bool _waiting_option = false){
        m_result = CMD_RESULT_MAX;
        if(_args != nullptr){
            if( !_waiting_option ){
                int16_t cmd_max_len = _len;
                int16_t cmd_start_indx = __strstr(_args, m_cmd, _len);
                int16_t cmd_end_indx = __strstr(_args+cmd_start_indx, " ");
                cmd_end_indx = cmd_end_indx < 0 ? cmd_max_len : (cmd_start_indx+cmd_end_indx);
                cmd_end_indx = cmd_end_indx > cmd_max_len ? cmd_max_len : cmd_end_indx;

                // check if command start and end indices are valid
                if( cmd_start_indx >= 0 && cmd_start_indx < cmd_end_indx && cmd_end_indx <= cmd_max_len ){
                    char argcmd[CMD_SIZE_MAX];
                    memset(argcmd, 0, CMD_SIZE_MAX);
                    memcpy(argcmd, _args+cmd_start_indx, abs(cmd_end_indx-cmd_start_indx));
                    if( isValidCommand(argcmd) ){
                        char argoptn[CMD_OPTION_SIZE_MAX];
                        // get the option start and end indices
                        int16_t optn_start_indx = cmd_end_indx;
                        int16_t optn_end_indx = __strstr(_args+optn_start_indx, CMD_OPTION_ASSIGN_OPERATOR);
                        optn_end_indx += optn_end_indx != -1 ? optn_start_indx : 0;
                        if( optn_end_indx != -1 && m_optionindx > 0 ){
                            if( optn_end_indx < cmd_max_len && optn_start_indx < optn_end_indx ){
                                do{
                                    memset(argoptn, 0, CMD_OPTION_SIZE_MAX);
                                    memcpy(argoptn, _args+optn_start_indx, optn_end_indx-optn_start_indx);
                                    // get the option value start and end indices
                                    int16_t optn_val_start_index = optn_end_indx+strlen(CMD_OPTION_ASSIGN_OPERATOR);
                                    int16_t optn_val_end_index = __strstr(_args+optn_val_start_index, m_optionseparator);
                                    optn_val_end_index += optn_val_end_index != -1 ? optn_val_start_index : cmd_max_len+1;
                                    char *argoptntrimmed = __strtrim(argoptn);
                                    int8_t validoptnindex = getOptionIndex(argoptntrimmed);
                                    if( validoptnindex != -1 ){
                                        m_options[validoptnindex].optionval = __strtrim(_args+optn_val_start_index);
                                        m_options[validoptnindex].optionvalsize = optn_val_end_index - optn_val_start_index;
                                        m_result = CMD_RESULT_OK;
                                    }else{
                                        m_result = CMD_RESULT_INVALID_OPTION;
                                        break;
                                    }
                                    // next option start index will start with last option value end index
                                    optn_start_indx = optn_val_end_index+strlen(m_optionseparator);
                                    // optn_start_indx += optn_start_indx != -1 ? (optn_val_end_index+strlen(m_optionseparator)) : 0;
                                    optn_end_indx = __strstr(_args+optn_start_indx, CMD_OPTION_ASSIGN_OPERATOR);
                                    optn_end_indx += optn_end_indx != -1 ? optn_start_indx : 0;
                                } while ( optn_start_indx > 0 && optn_end_indx > 0 && optn_end_indx < cmd_max_len && optn_start_indx < optn_end_indx);
                            }else{
                                m_result = CMD_RESULT_ARGS_ERROR;
                            }
                        }else{

                            if(m_acceptArgsOptions){

                                // if command has free options. 
                                uint8_t option_indx = 0;
                                int16_t optn_val_start_index = cmd_end_indx + 1;
                                int16_t optn_val_end_index = -1;

                                do{
                                    // get the option value start and end indices
                                    optn_val_end_index = __strstr(_args+optn_val_start_index, m_optionseparator);
                                    optn_val_end_index += optn_val_end_index != -1 ? optn_val_start_index : cmd_max_len+1;
                                    optn_val_end_index = optn_val_end_index > cmd_max_len ? cmd_max_len : optn_val_end_index;

                                    m_options[option_indx].optionval = __strtrim(_args+optn_val_start_index);
                                    m_options[option_indx++].optionvalsize = optn_val_end_index - optn_val_start_index;
                                    m_result = CMD_RESULT_OK;

                                    // next option value start index will start with last option value end index
                                    optn_val_start_index = optn_val_end_index+strlen(m_optionseparator);
                                } while ( optn_val_start_index > 0 && optn_val_end_index > 0 && optn_val_start_index < cmd_max_len && option_indx < CMD_OPTION_MAX);
                            }

                            // if command dont have any options by default
                            m_result = CMD_RESULT_OK;
                        }
                    }else{
                        m_result = CMD_RESULT_INVALID;
                    }
                }else{
                    m_result = CMD_RESULT_NOT_FOUND;
                }
            }else{
                if( m_waitingoptionindx != -1 && m_waitingoptionindx < m_optionindx ){
                    m_options[m_waitingoptionindx].optionval = _args;
                    m_options[m_waitingoptionindx].optionvalsize = _len;
                    m_waitingoptionindx = -1;
                    m_result = CMD_RESULT_OK;
                }else{
                }
            }
        }
        /* execute command if format is ok */
        if( CMD_RESULT_OK == m_result ){
            // if( nullptr != m_terminal ){
            // 	m_terminal->write_ro(RODT_ATTR("Executing cmd : "));
            // 	m_terminal->write(m_cmd);
            // 	m_terminal->write(RODT_ATTR("\n"));
            // }
            m_status = CMD_STATUS_ACTIVE;
            m_result = execute();
        }
        if( CMD_RESULT_INCOMPLETE != m_result ){
            m_status = CMD_STATUS_INACTIVE;
            ResultToTerminal(m_result);
            // once executed clear the options
            ClearOptions();
        }
        return m_result;
    }

    /**
     * @brief Clears the command data.
     */
    void Clear(){
        memset(m_cmd, 0, CMD_SIZE_MAX);
        ClearOptions(true);
        m_optionindx = 0;
        m_terminal = nullptr;
        m_status = CMD_STATUS_MAX;
        m_result = CMD_RESULT_MAX;
        m_acceptArgsOptions = false;
        m_optionseparator = CMD_OPTION_SEPERATOR_COMMA;
    }

    /**
     * @brief Clears the command options.
     * @param deep If true, clears the option names as well.
     */
    void ClearOptions(bool deep = false){
        for (uint8_t i = 0; i < CMD_OPTION_MAX; i++){
            m_options[i].Clear(deep);
        }
        if(deep)
            m_waitingoptionindx = -1;
    }

    /**
     * @brief Outputs the command result to the terminal.
     * @param res The result of the command execution.
     */
    void ResultToTerminal(cmd_result_t res){
        if( nullptr != m_terminal && CMD_RESULT_INCOMPLETE != res && CMD_RESULT_OK != res && !isWaitingForOption() ){
            m_terminal->writeln();
            switch (res){
            case CMD_RESULT_ARGS_ERROR:
                m_terminal->write_ro(RODT_ATTR("Arg Error"));
                break;
            case CMD_RESULT_ARGS_MISSING:
                m_terminal->write_ro(RODT_ATTR("Arg Missing"));
                break;
            case CMD_RESULT_NOT_FOUND:
                m_terminal->write_ro(RODT_ATTR("CMD Not Found"));
                break;
            case CMD_RESULT_INVALID:
                m_terminal->write_ro(RODT_ATTR("CMD invalid"));
                break;
            case CMD_RESULT_INVALID_OPTION:
                m_terminal->write_ro(RODT_ATTR("Option invalid"));
                break;
            case CMD_RESULT_NEED_AUTH:
                m_terminal->write_ro(RODT_ATTR("Required login"));
                break;
            case CMD_RESULT_WRONG_CREDENTIAL:
                m_terminal->write_ro(RODT_ATTR("Wrong Credential"));
                break;
            case CMD_RESULT_MAX:
                m_terminal->write_ro(RODT_ATTR("Unknown"));
                break;
            case CMD_RESULT_OK:
                // m_terminal->write_ro(RODT_ATTR("Success"));
                break;
            default:
                break;
            }
            m_terminal->writeln();
        }
    }

    /**
     * @brief Checks if the command requires authentication.
     * @return True if authentication is required, false otherwise.
     */
    virtual bool needauth() { return false; }

    /**
     * @brief Executes the command logic.
     * @return The result of the command execution.
     */
    virtual cmd_result_t execute() = 0;

} cmd_t;

#endif
