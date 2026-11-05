/******************************** Diff Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 1st Nov 2026
******************************************************************************/

#ifndef _DIFF_COMMAND_H_
#define _DIFF_COMMAND_H_

#include "CommandCommon.h"

#ifdef ENABLE_STORAGE_SERVICE

struct DiffCommand : public CommandBase {

	DiffCommand(){
		Clear();
		SetCommand(CMD_NAME_DIFF);
		setArgs(RODT_ATTR(""), 2);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_DIFF, [](void *arg)->void *{
			return pdiutil::safe_new<DiffCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("diff <file1> <file2>  the lines that differ, in the normal diff format");
	}

#ifdef ENABLE_AUTH_SERVICE
	bool needauth() override { return true; }
#endif

	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_ERROR_PERM;
		}
#endif

		if( nullptr == m_terminal ){
			return PDI_OK;
		}

		CommandOption *first = &m_options[0];
		CommandOption *second = &m_options[1];
		if( !optionGiven(first) || !optionGiven(second) ){
			return CMD_ERROR_ARGS_MISSING;
		}

		pdiutil::string names[2] = {
			pdiutil::string(first->optionval, (pdiutil::string::size_type)first->optionvalsize),
			pdiutil::string(second->optionval, (pdiutil::string::size_type)second->optionvalsize)
		};
		pdiutil::string paths[2] = { resolveArgPath(first), resolveArgPath(second) };

		m_terminal->putln();

		for( uint8_t f = 0; f < 2; f++ ){
			if( __i_fs.isDirectory(paths[f].c_str()) ){
				m_terminal->write_ro(RODT_ATTR("is a directory : "));
				m_terminal->write(names[f].c_str());
				return CMD_ERROR_FAILED;
			}
			if( !__i_fs.isFileExist(paths[f].c_str()) ){
				m_terminal->write_ro(RODT_ATTR(CMD_MSG_NOT_FOUND));
				writeName(names[f].c_str());
				return CMD_ERROR_FAILED;
			}
		}

		pdiutil::vector<uint32_t> lines[2];
		for( uint8_t f = 0; f < 2; f++ ){
			bool refused = false;
			int rc = readCommandLines(paths[f], m_terminal, [&](const pdiutil::string &line)->bool{
				uint32_t count = (uint32_t)lines[f].size();
				lines[f].push_back(__hash32(line.c_str(), (uint32_t)line.size()));
				refused = ( lines[f].size() != count + 1 || lines[f].size() > DIFF_LINES_MAX );
				return !refused;
			});
			if( rc < 0 ){
				writeFailure(RODT_ATTR(CMD_MSG_FAILED), names[f].c_str(), (int32_t)rc);
				return CMD_ERROR_FAILED;
			}
			if( refused ){
				m_terminal->write_ro(RODT_ATTR("too large to compare : "));
				m_terminal->write(names[f].c_str());
				return CMD_ERROR_FAILED;
			}
		}

		if( commandStopped(m_terminal) ){
			return stoppedResult(PDI_OK);
		}

		pdiutil::vector<uint16_t> trace;
		int32_t steps = shortestEdit(lines[0], lines[1], trace);
		if( steps < 0 ){
			m_terminal->write_ro(RODT_ATTR(CMD_MSG_OUT_OF_MEMORY));
			return CMD_ERROR_FAILED;
		}
		if( 0 == steps ){
			return PDI_OK;
		}

		pdiutil::vector<uint16_t> at[2];
		pdiutil::vector<uint8_t> kinds;
		if( !backtrack(lines[0], lines[1], trace, steps, at[0], at[1], kinds) ){
			m_terminal->write_ro(RODT_ATTR(CMD_MSG_OUT_OF_MEMORY));
			return CMD_ERROR_FAILED;
		}
		trace.clear();
		lines[0].clear();
		lines[1].clear();

		printHunks(paths, at, kinds);

		return stoppedResult(CMD_RESULT_FALSE);
	}

private:

	/**
	 * Runs the greedy shortest edit search over two hashed files and fills the
	 * trace of furthest reaches; the edit count, or -1 when memory ran out.
	 */
	int32_t shortestEdit(const pdiutil::vector<uint32_t> &a, const pdiutil::vector<uint32_t> &b, pdiutil::vector<uint16_t> &trace){
		int32_t n = (int32_t)a.size();
		int32_t m = (int32_t)b.size();

		for( int32_t d = 0; d <= n + m; d++ ){

			uint32_t base = (uint32_t)(d * d);
			for( int32_t i = 0; i < 2 * d + 1; i++ ){
				trace.push_back(0);
			}
			if( trace.size() != base + 2 * d + 1 ){
				return -1;
			}

			for( int32_t k = -d; k <= d; k += 2 ){
				int32_t x = 0;
				if( d > 0 ){
					x = goesDown(trace, d, k) ? reach(trace, d - 1, k + 1) : reach(trace, d - 1, k - 1) + 1;
				}
				int32_t y = x - k;
				while( x < n && y < m && a[x] == b[y] ){
					x++;
					y++;
				}
				trace[base + k + d] = (uint16_t)x;
				if( x >= n && y >= m ){
					return d;
				}
			}

			__i_dvc_ctrl.yield();
			if( commandStopped(m_terminal) ){
				return -1;
			}
		}
		return n + m;
	}

	/**
	 * The furthest reach recorded for diagonal k after d edits.
	 */
	int32_t reach(const pdiutil::vector<uint16_t> &trace, int32_t d, int32_t k){
		return (int32_t)trace[(uint32_t)(d * d) + k + d];
	}

	/**
	 * Whether diagonal k after d edits is reached by an insertion, coming down
	 * from diagonal k+1, rather than by a deletion from k-1.
	 */
	bool goesDown(const pdiutil::vector<uint16_t> &trace, int32_t d, int32_t k){
		return ( k == -d || ( k != d && reach(trace, d - 1, k - 1) < reach(trace, d - 1, k + 1) ) );
	}

	/**
	 * Walks the trace back from both ends and lists each edit in file order: a
	 * deletion of a first file line or an insertion of a second file line.
	 */
	bool backtrack(const pdiutil::vector<uint32_t> &a, const pdiutil::vector<uint32_t> &b, const pdiutil::vector<uint16_t> &trace,
				   int32_t steps, pdiutil::vector<uint16_t> &ata, pdiutil::vector<uint16_t> &atb, pdiutil::vector<uint8_t> &kinds){
		int32_t x = (int32_t)a.size();
		int32_t y = (int32_t)b.size();

		for( int32_t d = steps; d > 0; d-- ){
			int32_t k = x - y;
			bool down = goesDown(trace, d, k);
			int32_t pk = down ? k + 1 : k - 1;
			int32_t px = reach(trace, d - 1, pk);
			int32_t py = px - pk;
			ata.push_back((uint16_t)px);
			atb.push_back((uint16_t)py);
			kinds.push_back(down ? 1 : 0);
			x = px;
			y = py;
		}

		if( (int32_t)kinds.size() != steps || (int32_t)ata.size() != steps || (int32_t)atb.size() != steps ){
			return false;
		}

		for( int32_t i = 0, j = steps - 1; i < j; i++, j-- ){
			uint16_t ta = ata[i]; ata[i] = ata[j]; ata[j] = ta;
			uint16_t tb = atb[i]; atb[i] = atb[j]; atb[j] = tb;
			uint8_t tk = kinds[i]; kinds[i] = kinds[j]; kinds[j] = tk;
		}
		return true;
	}

	/**
	 * Groups adjacent edits into hunks and writes each one: its range line, the
	 * removed lines, a separator when it changes, and the added lines.
	 */
	void printHunks(pdiutil::string paths[2], pdiutil::vector<uint16_t> at[2], pdiutil::vector<uint8_t> &kinds){
		uint32_t lineno[2] = { 0, 0 };
		uint64_t offset[2] = { 0, 0 };
		uint32_t i = 0;

		while( i < kinds.size() && !commandStopped(m_terminal) ){

			uint32_t first = i;
			uint32_t deleted = 0;
			uint32_t inserted = 0;
			uint32_t ea = at[0][i];
			uint32_t eb = at[1][i];

			while( i < kinds.size() && at[0][i] == ea && at[1][i] == eb ){
				if( 1 == kinds[i] ){
					inserted++;
					eb++;
				}else{
					deleted++;
					ea++;
				}
				i++;
			}

			uint32_t sa = at[0][first];
			uint32_t sb = at[1][first];

			writeRange(sa, deleted);
			m_terminal->write(deleted && inserted ? 'c' : deleted ? 'd' : 'a');
			writeRange(sb, inserted);
			m_terminal->putln();

			pdiutil::string text;
			for( uint32_t l = 0; l < deleted; l++ ){
				lineAt(paths[0], lineno[0], offset[0], sa + l, text);
				m_terminal->write_ro(RODT_ATTR("< "));
				m_terminal->writeln(text.c_str());
			}
			if( deleted && inserted ){
				m_terminal->writeln_ro(RODT_ATTR("---"));
			}
			for( uint32_t l = 0; l < inserted; l++ ){
				lineAt(paths[1], lineno[1], offset[1], sb + l, text);
				m_terminal->write_ro(RODT_ATTR("> "));
				m_terminal->writeln(text.c_str());
			}

			__i_dvc_ctrl.yield();
		}
	}

	/**
	 * Writes a hunk side as diff does: the line before an empty side, one line
	 * number, or a first,last pair; numbers count from one.
	 */
	void writeRange(uint32_t start, uint32_t count){
		if( 0 == count ){
			m_terminal->write((int32_t)start);
			return;
		}
		m_terminal->write((int32_t)(start + 1));
		if( count > 1 ){
			m_terminal->write(',');
			m_terminal->write((int32_t)(start + count));
		}
	}

	/**
	 * Reads forward to the wanted line of a file, carrying the line number and
	 * offset across calls so each file is read once from start to end.
	 */
	void lineAt(const pdiutil::string &path, uint32_t &lineno, uint64_t &offset, uint32_t want, pdiutil::string &out){
		out.clear();
		while( lineno <= want ){
			out.clear();
			int bytes = __i_fs.readFile(path.c_str(), 200, [&](char *data, uint32_t size) -> bool {
				out.append(data, size);
				return true;
			}, offset, "\n");
			if( bytes < 0 ){
				out.clear();
				return;
			}
			offset += (uint64_t)bytes + 1;
			lineno++;
		}
		if( !out.empty() && '\r' == out.back() ){
			out.pop_back();
		}
	}
};

#endif

#endif
