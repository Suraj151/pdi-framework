/*************************** File Operation Command ***************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st June 2019
******************************************************************************/

#ifndef _FILE_OPERATION_FS_COMMAND_H_
#define _FILE_OPERATION_FS_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE
/**
 * file read operation command
 * 
 * e.g. if we want to read file in the system then we can execute command as below
 * cat <filename>
 * 
 */
struct FileReadCommand : public CommandBase {

	/* Constructor */
	FileReadCommand(){
		Clear();
		SetCommand(CMD_NAME_FILE_READ);
		setAcceptArgsOptions(true);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_FILE_READ, [](void *arg)->void *{
			return new FileReadCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("cat <file>  print file contents to terminal");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		cmd_result_t result = CMD_RESULT_OK;

		if(nullptr != m_terminal){
			// Get first option which must be the filename to read
			CommandOption *cmdoptn = &m_options[0];
			pdiutil::string filename = resolveArgPath(cmdoptn);
			if( !filename.empty() ){

				m_terminal->putln();
				int iStatus = __i_fs.readFile(filename.c_str(), 10, [&](char* data, uint32_t size)->bool{
					m_terminal->write(data, size);
					// return true to continue reading
					return true;
				});

				if (iStatus < 0) {
					result = CMD_RESULT_FAILED;
					m_terminal->putln();
					m_terminal->write_ro(RODT_ATTR("Failed : "));
					m_terminal->write(filename.c_str());
					m_terminal->write_ro(RODT_ATTR(" : "));
					m_terminal->write((int32_t)iStatus);
				}
			}else{
				result = CMD_RESULT_ARGS_ERROR;
			}
		}

		return result;
	}
};


/**
 * file write / edit operation command
 *
 * fedit <file>  opens a scrolling in-place line editor. The path is a
 * positional argument; a missing file is created empty. A working copy
 * (<file>.tmp) is edited line by line; up/down navigate and scroll, ENTER
 * splits the active line at the cursor, and ESC opens a menu to save (!w),
 * cancel (!c) or delete the current line (!d). Only the active line lives in
 * the session buffer; surrounding lines are read from the working file.
 */
#define FWRITE_WIN_ROWS   10   // visible line rows in the viewport
#define FWRITE_BACKSCAN   512 // max bytes scanned backward to find a line start

struct FileEditCommand : public CommandBase {

	/* Constructor */
	FileEditCommand(){
		Clear();
		SetCommand(CMD_NAME_FILE_EDIT);
		reservePositionalSlots(1);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_FILE_EDIT, [](void *arg)->void *{
			return new FileEditCommand();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("fedit <file>  file editor; ESC !w/!c/!d");
	}

	// Editor state (persists across iterations while the command waits)
	bool m_editing = false;             // editor session active
	bool m_escMenu = false;             // ESC save/cancel/delete prompt shown
	bool m_rendered = false;            // viewport painted at least once
	pdiutil::string m_origpath;         // original file path
	pdiutil::string m_tmppath;          // working copy path
	pdiutil::string m_activePristine;   // pristine text of active line (modified check)
	uint32_t m_topOffset = 0;           // byte offset of top visible line
	uint32_t m_topLineNo = 0;           // line number (0-based) of top visible line
	uint32_t m_curLineNo = 0;           // active line number
	uint32_t m_curOffset = 0;           // byte offset of active line start
	uint32_t m_rowOffset[FWRITE_WIN_ROWS]; // byte offset of each visible line
	uint8_t m_rowCount = 0;             // number of visible lines drawn

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* keep the preloaded active line in the session buffer while editing */
	bool preservesLineBuffer() override { return m_editing; }

	/* editor drives everything from execute(); keep this a pass-through */
	cmd_result_t executeTermInputAction(cmd_term_inseq_t terminputaction) override{
		return m_result;
	}

	/* execute command with provided options */
	cmd_result_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_RESULT_NEED_AUTH;
		}
#endif

		session_t *s = SessionManager::current();
		if( nullptr == m_terminal || nullptr == s ){
			return CMD_RESULT_TERMINAL_ERR;
		}

		// First phase: resolve the positional filename and open the editor
		if( !m_editing ){

			CommandOption *fopt = &m_options[0];
			bool haveFile = ( nullptr != fopt->optionval && fopt->optionvalsize > 0 );
			if( !haveFile ){
				setWaitingForOption((int8_t)0);
				m_terminal->putln();
				m_terminal->write_ro(RODT_ATTR("Enter Filename : "));
				return CMD_RESULT_INCOMPLETE;
			}

			pdiutil::string filepath = resolveArgPath(fopt);
			if( filepath.empty() ){
				return CMD_RESULT_ARGS_ERROR;
			}

			if( !openEditor(filepath, s) ){
				return CMD_RESULT_FAILED;
			}

			setWaitingForOption((int8_t)0);
			return CMD_RESULT_INCOMPLETE;
		}

		// ESC menu: interpret the typed token on ENTER
		if( m_escMenu ){
			if( terminputaction == CMD_TERM_INSEQ_ENTER ){
				pdiutil::string tok = s->m_linebuf;
				if( tok.find("!w") != pdiutil::string::npos ){
					return finalizeSave(s);
				}else if( tok.find("!c") != pdiutil::string::npos ){
					return cancelEdit(s);
				}else if( tok.find("!d") != pdiutil::string::npos ){
					m_escMenu = false;
					deleteLine(s);
				}else{
					m_escMenu = false;
					reloadActive(s);
					redraw(s);
				}
			}
			setWaitingForOption((int8_t)0);
			return CMD_RESULT_INCOMPLETE;
		}

		// Editing mode: act on navigation / structural keys
		switch( terminputaction ){
			case CMD_TERM_INSEQ_UP_ARROW:   moveUp(s);   break;
			case CMD_TERM_INSEQ_DOWN_ARROW: moveDown(s); break;
			case CMD_TERM_INSEQ_ENTER:      splitLine(s); break;
			case CMD_TERM_INSEQ_ESC:
			case CMD_TERM_INSEQ_CTRL_C:
			case CMD_TERM_INSEQ_CTRL_Z:     enterEscMenu(s); break;
			default: break;
		}

		setWaitingForOption((int8_t)0);
		return CMD_RESULT_INCOMPLETE;
	}

	/* Open the editor: create a working copy and preload line 0 */
	bool openEditor(const pdiutil::string &filepath, session_t *s){

		m_origpath = filepath;
		m_tmppath = filepath;
		m_tmppath += ".tmp";

		if( __i_fs.isFileExist(m_tmppath.c_str()) ){
			__i_fs.deleteFile(m_tmppath.c_str());
		}

		if( __i_fs.isFileExist(m_origpath.c_str()) ){
			if( __i_fs.copyFile(m_origpath.c_str(), m_tmppath.c_str()) < 0 ) return false;
		}else{
			if( __i_fs.createFile(m_tmppath.c_str(), "") < 0 ) return false;
		}

		m_editing = true;
		m_escMenu = false;
		m_rendered = false;
		m_topOffset = 0;
		m_topLineNo = 0;
		m_curLineNo = 0;

		pdiutil::string line0; uint32_t consumed; bool hadEOL;
		if( !readLineAt(0, line0, consumed, hadEOL) ){
			line0.clear();
		}
		preloadActive(s, line0, 0);
		redraw(s);
		return true;
	}

	/* Preload a line into the session buffer as the active editable line */
	void preloadActive(session_t *s, const pdiutil::string &content, uint32_t cursorcol){
		s->m_linebuf = content;
		s->m_cursor = (cursorcol > (uint32_t)content.size()) ? content.size() : cursorcol;
		m_activePristine = content;
	}

	/* Read one line from the working file starting at byteOffset.
	   content <- line text without trailing CR/LF, consumed <- bytes to next
	   line start (incl the newline), hadEOL <- whether a newline terminated it.
	   Returns false when byteOffset is at/after EOF. */
	bool readLineAt(uint32_t byteOffset, pdiutil::string &content, uint32_t &consumed, bool &hadEOL){

		content.clear();
		consumed = 0;
		hadEOL = false;

		int64_t fsz = __i_fs.getFileSize(m_tmppath.c_str());
		if( fsz < 0 || byteOffset >= (uint32_t)fsz ) return false;

		// Scan for '\n' ourselves so this works on any backend (some ignore
		// the readUntilMatchStr argument). Bytes up to the newline form the
		// line; the newline itself is counted in 'consumed' but not returned.
		pdiutil::string raw;
		bool foundnl = false;
		int bytes = __i_fs.readFile(m_tmppath.c_str(), 64, [&](char *d, uint32_t sz)->bool{
			for( uint32_t i = 0; i < sz; i++ ){
				if( d[i] == '\n' ){ foundnl = true; return false; }
				raw.push_back(d[i]);
			}
			return true;
		}, byteOffset);
		if( bytes < 0 ) return false;

		hadEOL = foundnl;
		consumed = (uint32_t)raw.size() + (foundnl ? 1 : 0);
		content = raw;
		if( !content.empty() && content.back() == '\r' ) content.pop_back();
		return true;
	}

	/* Find the byte offset of the line preceding the one starting at 'offset' */
	uint32_t prevLineOffset(uint32_t offset){

		if( offset == 0 ) return 0;

		uint32_t windowStart = (offset > FWRITE_BACKSCAN) ? (offset - FWRITE_BACKSCAN) : 0;
		uint32_t want = offset - windowStart;
		pdiutil::string buf;
		__i_fs.readFile(m_tmppath.c_str(), 64, [&](char *d, uint32_t sz)->bool{
			buf.append(d, sz);
			return buf.size() < want;
		}, windowStart);
		if( (uint32_t)buf.size() > want ) buf.resize(want);

		// buf == file[windowStart, offset); last char is the previous line's '\n'.
		if( buf.empty() ) return 0;
		int i = (int)buf.size() - 2;
		while( i >= 0 && buf[i] != '\n' ) i--;
		if( i >= 0 ) return windowStart + (uint32_t)i + 1;
		return windowStart;
	}

	/* Rebuild the working file replacing [spliceStart, spliceEnd) with insertData */
	void rebuildSplice(uint32_t spliceStart, uint32_t spliceEnd, const char *insertData, uint32_t insertLen){

		pdiutil::string scratch = m_tmppath;
		scratch += ".e";
		if( __i_fs.isFileExist(scratch.c_str()) ) __i_fs.deleteFile(scratch.c_str());
		if( __i_fs.createFile(scratch.c_str(), "") < 0 ) return;

		if( spliceStart > 0 ){
			uint32_t copied = 0;
			__i_fs.readFile(m_tmppath.c_str(), 128, [&](char *d, uint32_t sz)->bool{
				uint32_t take = sz;
				if( copied + take > spliceStart ) take = spliceStart - copied;
				if( take ) __i_fs.writeFile(scratch.c_str(), d, take, true);
				copied += take;
				return copied < spliceStart;
			}, 0);
		}

		if( insertData && insertLen ){
			__i_fs.writeFile(scratch.c_str(), insertData, insertLen, true);
		}

		__i_fs.readFile(m_tmppath.c_str(), 128, [&](char *d, uint32_t sz)->bool{
			__i_fs.writeFile(scratch.c_str(), d, sz, true);
			return true;
		}, spliceEnd);

		__i_fs.deleteFile(m_tmppath.c_str());
		__i_fs.rename(scratch.c_str(), m_tmppath.c_str());
	}

	/* Write the active line back to the working file if it was modified */
	void commitActiveIfModified(session_t *s){

		if( s->m_linebuf == m_activePristine ) return;

		pdiutil::string cur; uint32_t consumed; bool hadEOL;
		readLineAt(m_curOffset, cur, consumed, hadEOL);

		pdiutil::string newline = s->m_linebuf;
		if( hadEOL ) newline += "\r\n";

		rebuildSplice(m_curOffset, m_curOffset + consumed, newline.c_str(), newline.size());
		m_activePristine = s->m_linebuf;
	}

	/* Re-read the active line from file into the session buffer */
	void reloadActive(session_t *s){
		pdiutil::string cur; uint32_t consumed; bool hadEOL;
		readLineAt(m_curOffset, cur, consumed, hadEOL);
		preloadActive(s, cur, 0);
	}

	/* Move the active line down, scrolling the window at the bottom edge */
	void moveDown(session_t *s){

		commitActiveIfModified(s);

		pdiutil::string cur; uint32_t curConsumed; bool curEOL;
		if( !readLineAt(m_curOffset, cur, curConsumed, curEOL) ) return;

		uint32_t nextOff = m_curOffset + curConsumed;
		pdiutil::string next; uint32_t nc; bool ne;
		if( !readLineAt(nextOff, next, nc, ne) ) return; // no line below

		m_curLineNo++;
		if( m_curLineNo - m_topLineNo >= FWRITE_WIN_ROWS ){
			pdiutil::string t; uint32_t tc; bool te;
			readLineAt(m_topOffset, t, tc, te);
			m_topOffset += tc;
			m_topLineNo++;
		}
		preloadActive(s, next, 0);
		redraw(s);
	}

	/* Move the active line up, scrolling the window at the top edge */
	void moveUp(session_t *s){

		commitActiveIfModified(s);

		if( m_curLineNo == 0 ) return;

		uint32_t prevOff;
		if( m_curLineNo - m_topLineNo > 0 ){
			prevOff = m_rowOffset[(m_curLineNo - m_topLineNo) - 1];
		}else{
			prevOff = prevLineOffset(m_topOffset);
			m_topOffset = prevOff;
			m_topLineNo--;
		}
		m_curLineNo--;

		pdiutil::string prev; uint32_t pc; bool pe;
		readLineAt(prevOff, prev, pc, pe);
		preloadActive(s, prev, 0);
		redraw(s);
	}

	/* Split the active line at the cursor into two lines */
	void splitLine(session_t *s){

		uint32_t cursorpos = (s->m_cursor > (uint32_t)s->m_linebuf.size()) ? s->m_linebuf.size() : s->m_cursor;
		pdiutil::string left = s->m_linebuf.substr(0, cursorpos);
		pdiutil::string right = s->m_linebuf.substr(cursorpos);

		pdiutil::string cur; uint32_t consumed; bool hadEOL;
		readLineAt(m_curOffset, cur, consumed, hadEOL);

		pdiutil::string ins = left;
		ins += "\r\n";
		ins += right;
		if( hadEOL ) ins += "\r\n";

		rebuildSplice(m_curOffset, m_curOffset + consumed, ins.c_str(), ins.size());
		m_activePristine = left; // left part now on the original line

		m_curLineNo++;
		if( m_curLineNo - m_topLineNo >= FWRITE_WIN_ROWS ){
			pdiutil::string t; uint32_t tc; bool te;
			readLineAt(m_topOffset, t, tc, te);
			m_topOffset += tc;
			m_topLineNo++;
		}
		preloadActive(s, right, 0);
		redraw(s);
	}

	/* Delete the active line from the working file */
	void deleteLine(session_t *s){

		pdiutil::string cur; uint32_t consumed; bool hadEOL;
		readLineAt(m_curOffset, cur, consumed, hadEOL);
		rebuildSplice(m_curOffset, m_curOffset + consumed, nullptr, 0);

		pdiutil::string nxt; uint32_t nc; bool ne;
		if( readLineAt(m_curOffset, nxt, nc, ne) ){
			preloadActive(s, nxt, 0);
		}else if( m_curLineNo > 0 ){
			uint32_t prevOff = prevLineOffset(m_curOffset);
			m_curLineNo--;
			if( m_curLineNo < m_topLineNo ){
				m_topOffset = prevOff;
				m_topLineNo = m_curLineNo;
			}
			pdiutil::string prev; uint32_t pc; bool pe;
			readLineAt(prevOff, prev, pc, pe);
			preloadActive(s, prev, 0);
		}else{
			preloadActive(s, "", 0);
		}
		redraw(s);
	}

	/* Draw a full-width reverse-video bar (nano-style) at the given row */
	void writeBar(uint8_t row, const pdiutil::string &text){
		const uint16_t width = 80;
		pdiutil::string line = text;
		if( (uint16_t)line.size() < width ) line.resize(width, ' ');
		else line = line.substr(0, width);
		m_terminal->csi_cursor_move(1, row);
		m_terminal->csi_reverse_video();
		m_terminal->write(line.c_str());
		m_terminal->csi_reset_style_color();
	}

	/* Draw the bottom bar: the ESC hint, or the save/cancel/delete menu */
	void drawBottomBar(bool menu){
		pdiutil::string txt = menu ? "  !w save    !c cancel    !d delline    "
		                           : "  ESC  options    ";
		writeBar(FWRITE_WIN_ROWS + 4, txt);
		if( menu ){
			// leave the cursor after the options so the typed token shows there
			m_terminal->csi_cursor_move((uint8_t)(txt.size() + 1), FWRITE_WIN_ROWS + 4);
		}
	}

	/* Show the save/cancel/delete menu, committing the active line first */
	void enterEscMenu(session_t *s){
		commitActiveIfModified(s);
		m_escMenu = true;
		s->m_linebuf.clear();
		s->m_cursor = 0;
		drawBottomBar(true);
	}

	/* Persist the working copy over the original and close the editor.
	   The active line was already committed when the ESC menu opened; the
	   buffer now holds the "!w" token, so must not be committed again. */
	cmd_result_t finalizeSave(session_t *s){
		if( __i_fs.isFileExist(m_origpath.c_str()) ){
			__i_fs.deleteFile(m_origpath.c_str());
		}
		__i_fs.rename(m_tmppath.c_str(), m_origpath.c_str());
		return closeEditor(s, RODT_ATTR("saved"));
	}

	/* Discard the working copy and close the editor */
	cmd_result_t cancelEdit(session_t *s){
		if( __i_fs.isFileExist(m_tmppath.c_str()) ){
			__i_fs.deleteFile(m_tmppath.c_str());
		}
		return closeEditor(s, RODT_ATTR("cancelled"));
	}

	/* Tear down editor state and hand the terminal back to the shell */
	cmd_result_t closeEditor(session_t *s, const char *msg){
		m_editing = false;
		m_escMenu = false;
		setWaitingForOption((int8_t)-1);
		m_terminal->csi_erase_display();
		m_terminal->write_ro(RODT_ATTR("fedit "));
		m_terminal->write(m_origpath.c_str());
		m_terminal->write_ro(RODT_ATTR(" : "));
		m_terminal->write_ro(msg);
		s->m_linebuf.clear();
		s->m_cursor = 0;
		return CMD_RESULT_OK;
	}

	/* Repaint the viewport and place the cursor on the active line */
	void redraw(session_t *s){

		// Clear once on open so the viewport anchors at the screen top; later
		// redraws repaint each row in place (erase_in_line) without wiping the
		// whole terminal, so the cursor never jumps back to the first line.
		if( !m_rendered ){
			m_terminal->csi_erase_display();
			m_rendered = true;
		}

		// Row 1: nano-style status bar showing the file name
		pdiutil::string title = "  ";
		title += m_origpath;
		writeBar(1, title);

		uint32_t off = m_topOffset;
		uint32_t lineno = m_topLineNo;
		m_rowCount = 0;
		uint8_t activeTermRow = 0;
		bool atEnd = false;

		for( uint8_t i = 0; i < FWRITE_WIN_ROWS; i++ ){

			uint8_t termRow = i + 2; // header occupies row 1
			m_terminal->csi_cursor_move(1, termRow);
			m_terminal->csi_erase_in_line(2);

			if( atEnd ){
				// past the last line: paint a blank row to clear any stale text
				continue;
			}

			pdiutil::string linetext; uint32_t consumed; bool hadEOL;
			bool ok = readLineAt(off, linetext, consumed, hadEOL);

			if( !ok ){
				// no file line here; render the active line only if it is the
				// virtual line at EOF (empty file / freshly created)
				if( lineno == m_curLineNo ){
					m_terminal->write_ro(RODT_ATTR("> "));
					m_terminal->write(s->m_linebuf.c_str());
					activeTermRow = termRow;
					m_curOffset = off;
					m_rowOffset[m_rowCount++] = off;
				}
				atEnd = true;
				continue;
			}

			m_rowOffset[m_rowCount++] = off;

			if( lineno == m_curLineNo ){
				m_terminal->write_ro(RODT_ATTR("> "));
				m_terminal->write(s->m_linebuf.c_str());
				activeTermRow = termRow;
				m_curOffset = off;
			}else{
				m_terminal->write_ro(RODT_ATTR("  "));
				m_terminal->write(linetext.c_str());
			}

			off += consumed;
			lineno++;
		}

		// two blank separator rows, then the bottom hint bar
		m_terminal->csi_cursor_move(1, FWRITE_WIN_ROWS + 2);
		m_terminal->csi_erase_in_line(2);
		m_terminal->csi_cursor_move(1, FWRITE_WIN_ROWS + 3);
		m_terminal->csi_erase_in_line(2);
		drawBottomBar(false);

		if( activeTermRow ){
			m_terminal->csi_cursor_move((uint8_t)(3 + s->m_cursor), activeTermRow);
		}
	}
};

#endif

#endif
