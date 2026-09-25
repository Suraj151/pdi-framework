/******************************** Wget Command ********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 10th Sep 2026
******************************************************************************/

#ifndef _WGET_COMMAND_H_
#define _WGET_COMMAND_H_

#include "CommandCommon.h"

#if defined(ENABLE_HTTP_CLIENT) && defined(ENABLE_STORAGE_SERVICE)

#include <transports/http/HTTPClient.h>

/**
 * wget command
 * e.g. if we want to download a resource to a file, we can execute command as below
 * wget <file_path> <url>
 * the path may name a directory or be left out, in which case the file is taken
 * into the working directory under the name the url ends with
 */
struct WgetCommand : public CommandBase {

	/* Constructor */
	WgetCommand(){
		Clear();
		SetCommand(CMD_NAME_WGET);
		setAcceptArgsOptions(true);
		setCmdOptionSeparator(CMD_OPTION_SEPERATOR_SPACE);
	}

	/**
     * @brief Register the command.
     */
    static void RegisterCommand(){
		CommandBase::RegisterCommand(CMD_NAME_WGET, [](void *arg)->void *{
			return pdiutil::safe_new<WgetCommand>();
		});
	}

	const char* getUsage() const override {
		return RODT_ATTR("wget [path] <url>  download a http/https url to a file");
	}

#ifdef ENABLE_AUTH_SERVICE
	/* override the necesity of required permission */
	bool needauth() override { return true; }
#endif

	/* execute command with provided options */
	pdi_err_t execute(cmd_term_inseq_t terminputaction){

#ifdef ENABLE_AUTH_SERVICE
		// return in case authentication needed and not authorized yet
		if( needauth() && !__auth_service.getAuthorized()){
			return CMD_ERROR_PERM;
		}
#endif

		pdi_err_t result = PDI_OK;

		if(nullptr != m_terminal){
			// Get first option which may be the destination path and second option
			// which must be the url. a lone option is the url itself
			CommandOption *cmdoptn1 = &m_options[0];
			CommandOption *cmdoptn2 = &m_options[1];
			bool hasfirst = ( nullptr != cmdoptn1 && nullptr != cmdoptn1->optionval && cmdoptn1->optionvalsize > 0 );
			bool hassecond = ( nullptr != cmdoptn2 && nullptr != cmdoptn2->optionval && cmdoptn2->optionvalsize > 0 );

			if( hasfirst ){

				CommandOption *urloptn = hassecond ? cmdoptn2 : cmdoptn1;
				CommandOption *pathoptn = hassecond ? cmdoptn1 : nullptr;

				pdiutil::string url;
				url.append(urloptn->optionval, (pdiutil::string::size_type)urloptn->optionvalsize);

				bool validurl = false;
				pdiutil::string urlname;
				{
					http_req_t probe;
					validurl = probe.init(url.c_str());
					urlname = fileNameFromUri(probe.uri);
				}

				if( !validurl ){

					result = CMD_ERROR_INVAL;
					m_terminal->putln();
					m_terminal->writeln_ro(RODT_ATTR("not a http or https url"));

				}else{

					pdiutil::string destpath = resolveDestPath(pathoptn, urlname);

					if( destpath.empty() ){

						result = CMD_ERROR_INVAL;
						m_terminal->putln();
						m_terminal->writeln_ro(RODT_ATTR("url names no file, give a path"));

					}else if( !destNameFits(destpath) ){

						result = CMD_ERROR_INVAL;
						m_terminal->putln();
						m_terminal->write_ro(RODT_ATTR("file name is longer than "));
						m_terminal->write((int32_t)FILE_NAME_MAX_SIZE);
						m_terminal->write_ro(RODT_ATTR(", give a shorter path in argument"));

					}else{

						result = downloadTo(url, destpath);
					}
				}
			}else{
				result = CMD_ERROR_INVAL;
			}
		}

		return result;
	}

protected:

	/**
	 * The file name the url's path ends with, taken without any query or
	 * fragment. Empty where the url names no file of its own.
	 */
	pdiutil::string fileNameFromUri(const char *uri){

		pdiutil::string name;

		if( nullptr == uri ){
			return name;
		}

		int16_t len = (int16_t)strlen(uri);
		int16_t end = 0;
		while( end < len && '?' != uri[end] && '#' != uri[end] ){
			end++;
		}

		int16_t start = end;
		while( start > 0 && FILE_SEPARATOR[0] != uri[start-1] ){
			start--;
		}

		if( start < end ){
			name.append(uri + start, (pdiutil::string::size_type)(end - start));
		}

		return name;
	}

	/**
	 * Where the download lands, joining the url's own name onto a directory
	 * argument or onto the working directory when no path is given.
	 */
	pdiutil::string resolveDestPath(const CommandBase::CommandOption *pathoptn, const pdiutil::string &urlname){

		if( nullptr == pathoptn ){
			return resolveArgPathStr(urlname.c_str(), (int16_t)urlname.size());
		}

		pdiutil::string destpath = resolveArgPath(pathoptn);

		if( !destpath.empty() && __i_fs.isDirExist(destpath.c_str()) ){

			if( urlname.empty() ){
				return pdiutil::string();
			}

			__i_fs.appendFileSeparator(destpath);
			destpath.append(urlname);
		}

		return destpath;
	}

	/**
	 * Whether the name the path ends with is one the filesystem can hold, so an
	 * oversized name is refused before anything is fetched.
	 */
	bool destNameFits(const pdiutil::string &destpath){

		pdiutil::string::size_type at = destpath.find_last_of(FILE_SEPARATOR[0]);
		pdiutil::string::size_type start = (pdiutil::string::npos == at) ? 0 : at + 1;

		return (destpath.size() - start) <= FILE_NAME_MAX_SIZE;
	}

	static constexpr uint8_t PROGRESS_CELLS = 20;
	static constexpr uint32_t PROGRESS_STEP = 4096;

	/**
	 * Redraws the transfer line in place, as a filled bar where the length is
	 * known and as a running byte count where the server did not give one.
	 */
	void renderProgress(uint32_t done, uint32_t total){

		m_terminal->write('\r');

		if( 0 < total ){

			uint32_t pct = (uint32_t)(((uint64_t)done * 100) / total);
			uint32_t filled = (pct * PROGRESS_CELLS) / 100;

			m_terminal->write('[');

			for( uint32_t cell = 0; cell < PROGRESS_CELLS; cell++ ){
				m_terminal->write((char)((cell < filled) ? '=' : ' '));
			}

			m_terminal->write_ro(RODT_ATTR("] "));
			m_terminal->write_pad(pdiutil::to_string((int)pct).c_str(), 3, true);
			m_terminal->write_ro(RODT_ATTR("%  "));
			m_terminal->write((int32_t)done);
			m_terminal->write('/');
			m_terminal->write((int32_t)total);

		}else{

			m_terminal->write_ro(RODT_ATTR("downloaded "));
			m_terminal->write((int32_t)done);
		}

		m_terminal->write_ro(RODT_ATTR(" bytes   "));
	}

	/**
	 * Fetches the url into the path, replacing any file already there and
	 * leaving nothing behind when the transfer does not complete.
	 */
	pdi_err_t downloadTo(const pdiutil::string &url, const pdiutil::string &destpath){

		Http_Client *http = Http_Client::GetStaticInstance();

		if( nullptr == http ){
			return CMD_ERROR_FAILED;
		}

		if( __i_fs.isFileExist(destpath.c_str()) ){
			__i_fs.deleteFile(destpath.c_str());
		}

		// the client that carries this url is the http client's to choose
		http->SetClient(nullptr);

		pdiutil::string path = destpath;
		int writeerr = 0;
		bool hinted = false;
		uint32_t wanted = 0;
		uint32_t contentlen = 0;
		uint64_t available = 0;
		uint32_t done = 0;
		uint32_t nextmark = 0;
		int32_t lastpct = -1;

		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("Checking link.."));

		int64_t got = http->DownloadStream(url.c_str(), [&](const uint8_t *buf, uint32_t sz) -> bool {

			if( !hinted ){
				hinted = true;
				available = __i_fs.getFreeSize();
				contentlen = sz;
				if( 0 < sz && (uint64_t)sz > available ){
					wanted = sz;
					return false;
				}
				m_terminal->putln();
				renderProgress(0, contentlen);
				return true;
			}

			int written = __i_fs.writeFile(path.c_str(), (const char *)buf, sz, true);

			if( written != (int)sz ){
				writeerr = written;
				return false;
			}

			done += sz;

			if( 0 < contentlen ){

				int32_t pct = (int32_t)(((uint64_t)done * 100) / contentlen);

				if( pct != lastpct ){
					lastpct = pct;
					renderProgress(done, contentlen);
				}

			}else if( done >= nextmark ){

				nextmark = done + PROGRESS_STEP;
				renderProgress(done, 0);
			}

			return true;
		});

		if( 0 < wanted ){

			__i_fs.deleteFile(destpath.c_str());
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("not enough space: needs "));
			m_terminal->write((int32_t)wanted);
			m_terminal->write_ro(RODT_ATTR(" bytes, "));
			m_terminal->write((int32_t)available);
			m_terminal->write_ro(RODT_ATTR(" free"));
			return CMD_ERROR_FAILED;
		}

		if( 0 > writeerr ){

			__i_fs.deleteFile(destpath.c_str());
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to write: "));
			m_terminal->write(destpath.c_str());
			m_terminal->write_ro(RODT_ATTR(" : "));
			m_terminal->write((int32_t)writeerr);
			return CMD_ERROR_FAILED;
		}

		if( got <= 0 ){

			__i_fs.deleteFile(destpath.c_str());
			m_terminal->putln();
			m_terminal->write_ro(RODT_ATTR("Failed to download: "));
			m_terminal->write(url.c_str());
			m_terminal->write_ro(RODT_ATTR(" : "));
			m_terminal->write((int32_t)got);
			return CMD_ERROR_FAILED;
		}

		m_terminal->putln();
		m_terminal->write_ro(RODT_ATTR("saved "));
		m_terminal->write((int32_t)got);
		m_terminal->write_ro(RODT_ATTR(" bytes to "));
		m_terminal->write(destpath.c_str());

		return PDI_OK;
	}
};
#endif


#endif
