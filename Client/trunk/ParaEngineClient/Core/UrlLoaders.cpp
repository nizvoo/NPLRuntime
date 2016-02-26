//-----------------------------------------------------------------------------
// Class:	Url loaders
// Authors:	LiXizhi
// Emails:	LiXizhi@yeah.net
// Company: ParaEngine
// Date:	2009.9.1
// Desc: 
//-----------------------------------------------------------------------------
#include "ParaEngine.h"
#include "NPLHelper.h"
#include "AISimulator.h"
#include "NPLRuntime.h"
#include "UrlLoaders.h"
#include "AsyncLoader.h"

#ifdef PARAENGINE_CLIENT
#include "memdebug.h"
#endif

using namespace ParaEngine;

//////////////////////////////////////////////////////////////////////////
//
// CUrlLoader
//
//////////////////////////////////////////////////////////////////////////
ParaEngine::CUrlLoader::CUrlLoader( const string& url)
	: m_url(url), m_nEstimatedSizeInBytes(0)
{
}
ParaEngine::CUrlLoader::CUrlLoader()
	: m_nEstimatedSizeInBytes(0)
{
}

ParaEngine::CUrlLoader::~CUrlLoader()
{

}

const char* ParaEngine::CUrlLoader::GetFileName()
{
	return m_url.c_str();
}

void ParaEngine::CUrlLoader::SetUrl(const char* url)
{
	if(url)
		m_url = url;
}

HRESULT ParaEngine::CUrlLoader::Decompress( void** ppData, int* pcBytes )
{
	return S_OK;
}

void ParaEngine::CUrlLoader::CleanUp()
{
}

HRESULT ParaEngine::CUrlLoader::Destroy()
{
	return S_OK;
}

HRESULT ParaEngine::CUrlLoader::Load()
{
	return S_OK;
}

void ParaEngine::CUrlLoader::SetEstimatedSizeInBytes( int nSize )
{
	m_nEstimatedSizeInBytes = nSize;
}

int ParaEngine::CUrlLoader::GetEstimatedSizeInBytes()
{
	return m_nEstimatedSizeInBytes;
}

//////////////////////////////////////////////////////////////////////////
//
// CUrlProcessor
//
//////////////////////////////////////////////////////////////////////////
ParaEngine::CUrlProcessor::CUrlProcessor( )
	:m_pFormPost(0), m_nTimeOutTime(DEFAULT_TIME_OUT), m_nStartTime(0), m_responseCode(0), m_nLastProgressTime(0),
	m_pFormLast(0), m_pUserData(0), m_returnCode(CURLE_OK), m_type(URL_REQUEST_HTTP_AUTO), 
	m_nPriority(0), m_nStatus(URL_REQUEST_UNSTARTED), m_pfuncCallBack(0), m_nBytesReceived(0), 
	m_nTotalBytes(0), m_nUserDataType(0), m_pFile(NULL), m_pThreadLocalData(NULL), m_bForbidReuse(false), m_bEnableProgressUpdate(true)
{
}

ParaEngine::CUrlProcessor::CUrlProcessor(const string& url, const string& npl_callback)
	:m_pFormPost(0), m_nTimeOutTime(DEFAULT_TIME_OUT), m_nStartTime(0), m_responseCode(0), m_nLastProgressTime(0),
	m_pFormLast(0), m_pUserData(0), m_returnCode(CURLE_OK), m_type(URL_REQUEST_HTTP_AUTO), 
	m_nPriority(0), m_nStatus(URL_REQUEST_UNSTARTED), m_pfuncCallBack(0), m_nBytesReceived(0), 
	m_nTotalBytes(0), m_nUserDataType(0), m_pFile(NULL), m_pThreadLocalData(NULL), m_bEnableProgressUpdate(true)
{
	m_url = url;
	SetScriptCallback(npl_callback.c_str());
}


ParaEngine::CUrlProcessor::~CUrlProcessor()
{
	CleanUp();
}

void ParaEngine::CUrlProcessor::CleanUp()
{
	SAFE_DELETE(m_pFile);
	if(m_pFormPost)
	{
		/* then cleanup the form post chain */
		curl_formfree(m_pFormPost);
		m_pFormPost = NULL;
	}
	SafeDeleteUserData();
}

void ParaEngine::CUrlProcessor::SafeDeleteUserData()
{
	if(m_nUserDataType>0)
	{
		if(m_nUserDataType==1)
		{
			SAFE_DELETE(m_pUserData);
		}
	}
}

void CUrlProcessor::SetProcessorWorkerData(IProcessorWorkerData * pThreadLocalData )
{
	m_pThreadLocalData = pThreadLocalData;
}

ParaEngine::IProcessorWorkerData * CUrlProcessor::GetProcessorWorkerData()
{
	return m_pThreadLocalData;
}

HRESULT ParaEngine::CUrlProcessor::LockDeviceObject()
{
	return S_OK;
}

HRESULT ParaEngine::CUrlProcessor::UnLockDeviceObject()
{
	// complete the task
	CompleteTask();
	return S_OK;
}

HRESULT ParaEngine::CUrlProcessor::Destroy()
{
	return S_OK;
}

HRESULT ParaEngine::CUrlProcessor::Process( void* pData, int cBytes )
{
	// Let us do the easy way. 
	CURL *curl = NULL;
	
	bool bShareCurlHandle = false;
	// we will reuse the same easy handle per thread
	if(GetProcessorWorkerData() && GetProcessorWorkerData()->GetCurlInterface())
	{
		curl = (CURL*)(GetProcessorWorkerData()->GetCurlInterface());
		bShareCurlHandle = true;
	}
	else
	{
		curl = curl_easy_init();
	}
	if(curl) 
	{
		UpdateTime();

		SetCurlEasyOpt(curl);

		m_returnCode = curl_easy_perform(curl);

		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &m_responseCode);
		m_nStatus = CUrlProcessor::URL_REQUEST_COMPLETED;
		
		// SLEEP(2000);

		if(!bShareCurlHandle)
		{
			/* always cleanup */
			curl_easy_cleanup(curl);
		}
		return S_OK;
	}
	return E_FAIL;
}

HRESULT ParaEngine::CUrlProcessor::CopyToResource()
{
	return S_OK;
}

void ParaEngine::CUrlProcessor::SetResourceError()
{
}


void ParaEngine::CUrlProcessor::SetScriptCallback(const char* sCallback)
{
	if(sCallback)
	{
		
		m_sNPLStateName.clear();
		if(sCallback[0] == '(')
		{
			int i = 1;
			while( (sCallback[i]!=')') && (sCallback[i]!='\0'))
			{
				i++;
			}
			i++;
			if(sCallback[i-1]!='\0')
			{
				m_sNPLCallback = sCallback+i;
			}
			if( i>2 && !(i==4 && (sCallback[1]=='g') && (sCallback[2]=='l')) )
				m_sNPLStateName.assign(sCallback+1, i-2);
		}
		else
		{
			m_sNPLCallback = sCallback;
		}
	}
}

void ParaEngine::CUrlProcessor::SetSaveToFile(const char* filename)
{
	if(filename)
		m_sSaveToFileName = filename;
}


void ParaEngine::CUrlProcessor::SetUrl(const char* url)
{
	if(url)
		m_url = url;
}

void ParaEngine::CUrlProcessor::SetHeadersOnly()
{
	m_type = URL_REQUEST_HTTP_HEADERS_ONLY;
}

void ParaEngine::CUrlProcessor::SetForbidReuse(bool bForbidReuse)
{
	m_bForbidReuse = bForbidReuse;
}

int ParaEngine::CUrlProcessor::GetTotalBytes()
{
	//ParaEngine::Lock lock_(m_mutex);
	int nTotalBytes = m_nTotalBytes;
	return nTotalBytes;
}

int ParaEngine::CUrlProcessor::GetBytesReceived()
{
	//ParaEngine::Lock lock_(m_mutex);
	int nBytesReceived = m_nBytesReceived;
	return nBytesReceived;
}

void ParaEngine::CUrlProcessor::AddBytesReceived(int nByteCount)
{
	//ParaEngine::Lock lock_(m_mutex);
	m_nBytesReceived += nByteCount;

	if(m_pThreadLocalData)
	{
		m_pThreadLocalData->AddBytesProcessed(nByteCount);
	}
}


void ParaEngine::CUrlProcessor::SetCurlEasyOpt( CURL* handle )
{
	// reset data 
	m_data.clear();
	m_header.clear();
	m_nLastProgressTime = 0;
	curl_easy_setopt(handle, CURLOPT_URL, m_url.c_str());

	/* Define our callback to get called when there's data to be written */
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, CUrl_write_data_callback);
	/* Set a pointer to our struct to pass to the callback */
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);

	curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, CUrl_write_header_callback);
	curl_easy_setopt(handle, CURLOPT_HEADERDATA, this);

	/* Pass a long. Set to 1 to make the next transfer explicitly close the connection when done. Normally, libcurl keeps all connections alive when done with one transfer in case a succeeding one follows that can re-use them. */
	curl_easy_setopt(handle, CURLOPT_FORBID_REUSE, m_bForbidReuse ? 1 : 0);

	/* do not verify host */
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0); 
	curl_easy_setopt(handle,CURLOPT_SSL_VERIFYPEER, 0);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	
	//curl_easy_setopt(handle,CURLOPT_CAINFO, NULL);
	//curl_easy_setopt(handle,CURLOPT_CAPATH, NULL); 

	/**
	Pass a long. It should contain the maximum time in seconds that you allow the connection to the server to take. 
	This only limits the connection phase, once it has connected, this option is of no more use. Set to zero to disable 
	connection timeout (it will then only timeout on the system's internal timeouts). See also the CURLOPT_TIMEOUT option
	*/

	// time allowed to connect to server
	curl_easy_setopt(handle,CURLOPT_CONNECTTIMEOUT,5);
	// progress callback is only used to terminate the url progress
	curl_easy_setopt(handle,CURLOPT_NOPROGRESS, 0); 
	curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, CUrl_progress_callback);
	curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, this);
	
	// The official doc says if multi-threaded use, this one should be set to 1. otherwise it may crash on certain conditions. 
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);

	// form if any. 
	if(m_pFormPost)
	{
		curl_easy_setopt(handle, CURLOPT_HTTPPOST, m_pFormPost);
	}
	else
	{
		curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
	}
	if (m_type == URL_REQUEST_HTTP_HEADERS_ONLY)
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
	else
		curl_easy_setopt(handle, CURLOPT_NOBODY, 0);
}


size_t ParaEngine::CUrlProcessor::write_data_callback( void *buffer, size_t size, size_t nmemb )
{
	UpdateTime();
	int nByteCount = (int)size*(int)nmemb;
	if(nByteCount>0)
	{
		AddBytesReceived(nByteCount);
		
		if(m_sSaveToFileName.empty())
		{
			// save to memory if no disk file name is provided
			int nOldSize = (int)m_data.size();
			m_data.resize(nOldSize+nByteCount);
			memcpy(&(m_data[nOldSize]), buffer, nByteCount);
		}
		else
		{
			if(m_pFile == 0)
			{
				m_pFile = new CParaFile();
				if(!m_pFile->CreateNewFile(m_sSaveToFileName.c_str(), true))
				{
					OUTPUT_LOG("warning: Failed create new file %s\n", m_sSaveToFileName.c_str());
				}
			}
			m_pFile->write((const char*)buffer, (int)nByteCount);
		}
		// just for testing: remove this, dump to debug. 
		// ParaEngine::CLogger::GetSingleton().Write((const char*)buffer, (int)nByteCount);
	}
	// SLEEP(200);
	if(CAsyncLoader::GetSingleton().interruption_requested())
		return 0;
	return nByteCount;
}

size_t ParaEngine::CUrlProcessor::CUrl_write_data_callback( void *buffer, size_t size, size_t nmemb, void *stream )
{
	CUrlProcessor * pTask=(CUrlProcessor *) stream;
	if(pTask) 
	{
		return pTask->write_data_callback(buffer, size, nmemb);
	}
	// If that amount differs from the amount passed to your function, it'll signal an error to the library and 
	// it will abort the transfer and return CURLE_WRITE_ERROR. 
	return 0;
}


int ParaEngine::CUrlProcessor::CUrl_progress_callback( void *clientp,double dltotal,double dlnow,double ultotal, double ulnow )
{
	if(CAsyncLoader::GetSingleton().interruption_requested())
		return -1;
	if (clientp)
	{
		CUrlProcessor* pUrlProcessor = (CUrlProcessor*)clientp;
		if (pUrlProcessor->IsEnableProgressUpdate())
			return pUrlProcessor->progress_callback(dltotal, dlnow, ultotal, ulnow);
	}
	return 0;
}


int ParaEngine::CUrlProcessor::progress_callback(double dltotal, double dlnow, double ultotal, double ulnow)
{
	DWORD nCurTime = ::GetTickCount();
	const int nMinProgressCallbackInterval = 500;
	if ((nCurTime - m_nLastProgressTime) > nMinProgressCallbackInterval)
	{
		m_nLastProgressTime = nCurTime;
		// OUTPUT_LOG("file %s: %d/%d \n", m_url.c_str(), (int)dlnow, (int)dltotal);
		if (!m_sNPLCallback.empty())
		{
			NPL::CNPLWriter writer;
			writer.WriteName("msg");
			writer.BeginTable();
			if (!m_sSaveToFileName.empty())
			{
				// if a disk file name is specified, we shall also insert following field info to the message struct. 
				// {DownloadState=""|"complete"|"terminated", totalFileSize=number, currentFileSize=number, PercentDone=number}
				writer.WriteName("DownloadState");
				writer.WriteValue("");
				writer.WriteName("totalFileSize");
				writer.WriteValue((int)dltotal);
				writer.WriteName("currentFileSize");
				writer.WriteValue((int)dlnow);
				writer.WriteName("PercentDone");
				writer.WriteValue((float)(dlnow / dltotal));
			}
			writer.EndTable();
			writer.WriteParamDelimiter();
			writer.Append(m_sNPLCallback.c_str());

			NPL::NPLRuntimeState_ptr pState = CGlobals::GetNPLRuntime()->GetRuntimeState(m_sNPLStateName);
			if (pState)
			{
				// thread safe
				pState->activate(NULL, writer.ToString().c_str(), (int)(writer.ToString().size()));
			}
		}
	}
	return 0;
}

size_t ParaEngine::CUrlProcessor::write_header_callback( void *buffer, size_t size, size_t nmemb )
{
	UpdateTime();
	int nByteCount = (int)size*(int)nmemb;
	if(nByteCount>0)
	{
		int nOldSize = (int)m_header.size();
		m_header.resize(nOldSize+nByteCount);
		memcpy(&(m_header[nOldSize]), buffer, nByteCount);
	}
	if(CAsyncLoader::GetSingleton().interruption_requested())
		return -1;
	return nByteCount;
}

size_t ParaEngine::CUrlProcessor::CUrl_write_header_callback( void *buffer, size_t size, size_t nmemb, void *stream )
{
	CUrlProcessor * pTask=(CUrlProcessor *) stream;
	if(pTask) 
	{
		return pTask->write_header_callback(buffer, size, nmemb);
	}
	// The callback function must return the number of bytes actually taken care of, or 
	// return -1 to signal error to the library (it will cause it to abort the transfer with 
	// a CURLE_WRITE_ERROR return code
	return -1;
}

void ParaEngine::CUrlProcessor::CompleteTask()
{
	// complete the request, call the callback and delete the task and free the handle
#ifdef _DEBUG
	OUTPUT_LOG("-->: URL request task %s completed \n", m_url.c_str());
#endif
	CAsyncLoader::GetSingleton().RemovePendingRequest(m_url.c_str());

	if(m_pFile)
	{
		m_pFile->close();
		SAFE_DELETE(m_pFile);
	}

	if(!m_sNPLCallback.empty())
	{
		NPL::CNPLWriter writer;
		writer.WriteName("msg");
		writer.BeginTable();

		if(!m_header.empty())
		{
			writer.WriteName("header");
			writer.WriteValue((const char*)(&(m_header[0])), (int)m_header.size());
		}

		if(!m_data.empty())
		{
			writer.WriteName("data");
			writer.WriteValue((const char*)(&(m_data[0])), (int)m_data.size());
		}

		if(!m_sSaveToFileName.empty())
		{
			// if a disk file name is specified, we shall also insert following field info to the message struct. 
			// {DownloadState=""|"complete"|"terminated", totalFileSize=number, currentFileSize=number, PercentDone=number}
			writer.WriteName("DownloadState");
			if(m_responseCode == 200)
			{
				writer.WriteValue("complete");
				CAsyncLoader::GetSingleton().log(string("NPL.AsyncDownload Completed:")+m_url+"\n");
			}
			else
			{
				writer.WriteValue("terminated");
				CAsyncLoader::GetSingleton().log(string("NPL.AsyncDownload Failed:")+m_url+"\n");
			}
			writer.WriteName("totalFileSize");
			writer.WriteValue(m_nBytesReceived);
			writer.WriteName("currentFileSize");
			writer.WriteValue(m_nBytesReceived);
			writer.WriteName("PercentDone");
			writer.WriteValue(100);
		}
		

		writer.WriteName("code");
		writer.WriteValue(m_returnCode);
		writer.WriteName("rcode");
		writer.WriteValue(m_responseCode);
		writer.EndTable();
		writer.WriteParamDelimiter();
		writer.Append(m_sNPLCallback.c_str());

		// TODO: 
		
		if(! m_sNPLStateName.empty())
		{
			NPL::NPLRuntimeState_ptr pState = CGlobals::GetNPLRuntime()->GetRuntimeState(m_sNPLStateName);
			if(pState) 
			{
				pState->DoString(writer.ToString().c_str(), (int)(writer.ToString().size()));
			}
		}
		else
		{
			CGlobals::GetAISim()->NPLDoString(writer.ToString().c_str(), (int)(writer.ToString().size()));
		}
	}
	if(m_pfuncCallBack)
	{
		m_pfuncCallBack(m_returnCode, this, this->m_pUserData);
	}
}


CURLFORMcode ParaEngine::CUrlProcessor::AppendFormParam( const char* name, const char* value )
{
	return curl_formadd(&m_pFormPost, &m_pFormLast, CURLFORM_COPYNAME, name, CURLFORM_COPYCONTENTS, value, CURLFORM_END);
}

CURLFORMcode ParaEngine::CUrlProcessor::AppendFormParam(const char* name, const char* type, const char* file, const char* data, int datalen, bool bCacheData)
{
	CURLFORMcode rc = CURL_FORMADD_OK;
	if (bCacheData)
	{
		data = CopyRequestData(data, datalen);
	}

	/* file upload */
	if ((file != NULL) && (data == NULL)) 
	{
		rc = (type == NULL)?
			curl_formadd(&m_pFormPost, &m_pFormLast, CURLFORM_COPYNAME, name, 
			CURLFORM_FILE, file, CURLFORM_END): 
		curl_formadd(&m_pFormPost, &m_pFormLast, CURLFORM_COPYNAME, name, 
			CURLFORM_FILE, file, 
			CURLFORM_CONTENTTYPE, type, CURLFORM_END); 
	}
	/* data field */
	else if ((file != NULL) && (data != NULL)) 
	{
		/* Add a buffer to upload */
		rc = (type != NULL)? 
			curl_formadd(&m_pFormPost, &m_pFormLast,
			CURLFORM_COPYNAME, name,
			CURLFORM_BUFFER, file, CURLFORM_BUFFERPTR, data, CURLFORM_BUFFERLENGTH, datalen,
			CURLFORM_CONTENTTYPE, type, 
			CURLFORM_END):
		curl_formadd(&m_pFormPost, &m_pFormLast,
			CURLFORM_COPYNAME, name,
			CURLFORM_BUFFER, file, CURLFORM_BUFFERPTR, data, CURLFORM_BUFFERLENGTH, datalen,
			CURLFORM_END);
	}
	else 
	{
		OUTPUT_LOG("warning: Mandatory: \"file\" \n");
	}
	if (rc != CURL_FORMADD_OK) 
	{
		OUTPUT_LOG("warning:  cannot add form: %d \n", rc);
	}
	return rc;
}

void ParaEngine::CUrlProcessor::SetCallBack(URL_LOADER_CALLBACK pFuncCallback, CUrlProcessorUserData* pUserData, bool bDeleteUserData)
{
	m_pfuncCallBack = pFuncCallback;
	m_pUserData = pUserData;
	m_nUserDataType = bDeleteUserData ?  1 : 0;
}

bool ParaEngine::CUrlProcessor::IsTimedOut( DWORD nCurrentTime )
{
	return ( ( m_nStartTime+m_nTimeOutTime) < nCurrentTime);
}

DWORD ParaEngine::CUrlProcessor::UpdateTime()
{
	m_nStartTime = ::GetTickCount();
	return m_nStartTime;
}

void ParaEngine::CUrlProcessor::SetTimeOut( int nMilliSeconds )
{
	m_nTimeOutTime = nMilliSeconds;
}

int ParaEngine::CUrlProcessor::GetTimeOut()
{
	return m_nTimeOutTime;
}

bool ParaEngine::CUrlProcessor::IsEnableProgressUpdate() const
{
	return m_bEnableProgressUpdate;
}

void ParaEngine::CUrlProcessor::SetEnableProgressUpdate(bool val)
{
	m_bEnableProgressUpdate = val;
}

const char* ParaEngine::CUrlProcessor::CopyRequestData(const char* pData, int nLength)
{
	if (!m_sRequestData.empty())
	{
		OUTPUT_LOG("error: only one copied data can be used in url request\n");
		// TODO: i may need an array of m_sRequestData to store multiple copied request data. 
	}
	m_sRequestData = std::string(pData, nLength);
	return m_sRequestData.c_str();
}
