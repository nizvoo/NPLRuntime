#pragma once
#include "IAttributeFields.h"	
#include "ZipArchive.h"

namespace ParaEngine
{
	/**
	* creating zip files
	* 
	* e.g.
	*  (1) Traditional use, creating a zipfile from existing files
	* CZipWriter* writer = CZipWriter::CreateZip("c:\\simple1.zip","");
	* writer->ZipAdd("znsimple.bmp", "c:\\simple.bmp");
	* writer->ZipAdd("znsimple.txt", "c:\\simple.txt");
	* writer->close();
	*/
	class CZipWriter : public IAttributeFields
	{
	public:
		CZipWriter();
		CZipWriter(void* handle);
		~CZipWriter();

		ATTRIBUTE_DEFINE_CLASS(CZipWriter);
		ATTRIBUTE_SUPPORT_CREATE_FACTORY(CZipWriter);

	public:
		/** 
		* call this to start the creation of a zip file.
		* one need to call Release()
		*/
		static CZipWriter* CreateZip(const char *fn, const char *password);

		/**
		* add a zip file to the zip. file call this for each file to be added to the zip.
		* @return: 0 if succeed.
		*/
		DWORD ZipAdd(const char* dstzn, const char* fn);
		/**
		* add a zip folder to the zip file. call this for each folder to be added to the zip.
		* @return: 0 if succeed.
		*/
		DWORD ZipAddFolder(const char* dstzn);

		/**
		* add everything in side a directory to the zip. 
		* e.g. AddDirectory("myworld/", "worlds/myworld/*.*", 10);
		* @param dstzn: all files in fn will be appended with this string to be saved in the zip file.
		* @param filepattern: file patterns, which can include wild characters in the file portion.
		* @param nSubLevel: sub directory levels. 0 means only files at parent directory.
		*/
		DWORD AddDirectory(const char* dstzn, const char* filepattern, int nSubLevel=0);

		/**
		* call this when you have finished adding files and folders to the zip file.
		* Note: you can't add any more after calling this.
		*/
		DWORD close();

	public:
		void* m_handle;
	};
}