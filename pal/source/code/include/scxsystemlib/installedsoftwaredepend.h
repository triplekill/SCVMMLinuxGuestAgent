/**
 *  Copyright (c) Microsoft Corporation
 *
 *  All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy
 *  of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 *  THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
 *  WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
 *  MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 *  See the Apache Version 2.0 License for specific language governing
 *  permissions and limitations under the License.
 *
 **/

/**
\file        

\brief      installedSoftware dependancy.

\date       2011-01-18 14:56:20

*/
/*----------------------------------------------------------------------------*/
#ifndef INSTALLEDSOFTWAREDEPENDENCIES_H
#define INSTALLEDSOFTWAREDEPENDENCIES_H

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/scxlog.h>
#include <stdio.h>
#include <vector>
#include <map>

#if defined(linux)
#include <scxcorelib/scxglob.h>
#include <dlfcn.h>
extern "C" { 
#include <rpm/rpmcli.h> 
#include <rpm/rpmds.h> 
#include <rpm/rpmts.h> 
}

#endif

namespace SCXSystemLib
{
    class SoftwareDependencies
    {
    public:
        virtual std::wstring GetDPKGStatusLocation()
        {
            return L"/var/lib/dpkg/status";
        }
        
        virtual ~SoftwareDependencies() { };
    };

#if defined(linux) && defined(PF_DISTRO_ULINUX)
    /*
      LibHandle is a wrapper class for the dl* functions used for dynamic loading of dynamic libraries.
     */
    class LibHandle
    {
        void * handle; //!< The wrapped object
        
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor
        */
        LibHandle()
        {
            handle = NULL;
        };

        /*----------------------------------------------------------------------------*/
        /**
           Constructor
           \param s : the full library path to open
        */
        LibHandle(const char * s)
        {
            Open(s);
        };

        /*----------------------------------------------------------------------------*/
        /**
           Closes the current handle if it is open
           \returns 0 if no handle is open, otherwise it returns the value from dlclose
               which returns 0 if successful, non-zero if error
           \note The caller of Close() will need to check the return value to determine if
               there was an error, and then perform the appropriate action.
        */
        int Close()
        {
            if (handle != NULL)
            {                
                int retval = dlclose(handle);
                handle = NULL;
                return retval;
            }
            return 0;
        };

        /*----------------------------------------------------------------------------*/
        /**
           Wrapper for dlopen
           \param s : the full library path to open
           \note If a library is open, please call Close() before any calls to Open(),
               otherwise handle will leak.
        */
        void Open(const char * s)
        {
            handle = dlopen(s, RTLD_NOW);
        };
        
        /*----------------------------------------------------------------------------*/
        /**
           A wrapper for the "dlsym" function.
           \param s : the full library path to open
           \returns what dlsym returns (a pointer to the symbol specified by "s". if not found, NULL)
        */
        void * GetSymbol(const char * s)
        {
            return dlsym(handle, s);
        };

        /*----------------------------------------------------------------------------*/
        /**
           A wrapper for the "dlerror" function.
           \returns what dlerror returns, which is either NULL if the previous dl* command was successful
               or a pointer to a char which contains the error message string associated with why it failed
        */
        char * GetError()
        {
            return dlerror();
        };

        /*----------------------------------------------------------------------------*/
        /**
           Used to determine if opening a library was succesful.
           \returns true if open, false if not open
        */
        bool IsOpen()
        {
            if (handle != NULL)
            {
                return true;
            }
            else
            {
                return false;
            }
        };

        /*----------------------------------------------------------------------------*/
        /**
           Virtual destructor, closes any open handle
        */
        virtual ~LibHandle()
        {
            Close();
        };
    };

    /*
      Acts as a container for all of the symbols loaded from the rpm dynamic library
     */
    struct rpmAPI 
    {
        LibHandle m_handle;  //!< contains the LibHandle for the rpm library
        struct poptOption * m_rpmQueryPoptTable; //!< contains the loaded symbol rpmQueryPoptTable
        struct poptOption * m_rpmQVSourcePoptTable; //!< contains the loaded symbol rpmQVSourcePoptTable
        struct rpmQVKArguments_s * m_rpmQVKArgs; //!< contains the loaded symbol rpmQVKArgs
        poptContext (*mf_rpmcliInit)(int argc, char *const argv[], struct poptOption * optionsTable); //!< contains the loaded function symbol rpmcliInit
        rpmts (*mf_rpmtsCreate)(void); //!< contains the loaded function symbol rpmtsCreate
        poptContext (*mf_rpmcliFini)(poptContext optCon); //!< contains the loaded function symbol rpmcliFini
        int (*mf_rpmcliQuery)(rpmts ts, QVA_t qva, const char ** argv); //!< contains the loaded function symbol rpmcliQuery
        rpmts (*mf_rpmtsFree)(rpmts ts); //!< contains the loaded function symbol rpmtsFree
    };

    /*
      Acts as a container for all of the symbols loaded from the popt dynamic library
     */
    struct poptAPI
    {
        LibHandle m_handle; //!< contains the LibHandle for the popt library
        struct poptOption * m_poptAliasOptions; //!< contains the loaded symbol poptAliasOptions
        struct poptOption * m_poptHelpOptions; //!< contains the loaded symbol poptHelpOptions
        const char ** (*mf_poptGetArgs)(poptContext con); //!< contains the loaded function symbol poptGetArgs
    };

    static rpmAPI gs_rpm;   //!< A global static variable containing the dynamically opened rpm library symbols
    static poptAPI gs_popt; //!< A global static variable containing the dynamically opened popt library symbols

#endif
         
    class InstalledSoftwareDependencies
    {
    public:

        /*----------------------------------------------------------------------------*/
        /**
           Constructor
           \param deps : used for dependency injection unit tests, defaults to normal behavior
        */
        InstalledSoftwareDependencies(SCXCoreLib::SCXHandle<SoftwareDependencies> deps = SCXCoreLib::SCXHandle<SoftwareDependencies>(new SoftwareDependencies()));
        /*----------------------------------------------------------------------------*/
        /**
           Virtual destructor
        */
        virtual ~InstalledSoftwareDependencies();
        /*----------------------------------------------------------------------------*/
        /**
        Init running context. 
        */
        void Init();

        /*----------------------------------------------------------------------------*/
        /**
        Clean up running context.
        */
        void CleanUp();

        /*----------------------------------------------------------------------------*/
        /**
        Get All installed software Ids,
        on linux, id will be display name since it's unique and we can get it from RPM API
        on solaris, id will be the name of folder where pkginfo stored.
        */
        virtual void GetInstalledSoftwareIds(std::vector<std::wstring>& ids);

#if defined(linux)
    public:
        /*----------------------------------------------------------------------------*/
        /**
        pass "-qi softwareName" params to RPM API and retrun the raw data about the software.
        \param softwareName : the productName or displayName of softwareName
        \param contents : Software information date retured by RPM API
        */
        virtual void GetSoftwareInfoRawData(const std::wstring& softwareName, std::vector<std::wstring>& contents);
#endif

#if defined(sun)
    public:
        /*----------------------------------------------------------------------------*/
        /**
        Read the pkginfo file and returns all lines.
        \param pkgFile : the path of pkginfo file.
        \param allLines : the content of pkginfo file
        */
        virtual void GetAllLinesOfPkgInfo(const std::wstring& pkgFile, std::vector<std::wstring>& allLines);
#endif

#if defined(hpux)
    public:
        typedef std::map<std::wstring, std::wstring> PROPMAP;

        static const std::wstring keyPublisher;
        static const std::wstring keyTag;
        static const std::wstring keyRevision;
        static const std::wstring keyTitle;
        static const std::wstring keyInstallDate;
        static const std::wstring keyInstallSource;
        static const std::wstring keyDirectory;

        /*----------------------------------------------------------------------------*/
        /**
        Read the index file and returns all lines.
        \param indexFile : the path of INDEX file.
        \param allProperties : the content of INDEX file
        */
        virtual bool GetAllPropertiesOfIndexFile(const std::wstring& indexFile, PROPMAP & allProperties);

    private:
        bool SkipSeparator(std::wifstream& fsIndex, wchar_t& sep);

#endif

#if defined(aix)
    public:

        /*----------------------------------------------------------------------------*/
        /**
        Collect properties beloning to productID.
        \param productID   : key identifier.  Used to search for install date in m_History.
        \param version     : [out] property
        \param description : [out] property
        \param installDate : [out] property
        \returns           True = Success, False = Fail
        */
        bool GetProperties(const std::wstring& productID, std::wstring& version, std::wstring& description, SCXCoreLib::SCXCalendarTime& installDate);

    private:
        /*----------------------------------------------------------------------------*/
        /**
        Tokenize out the product ID (2nd) field from the colon separated values of
        lpp output.  This is a lighter, cheaper version of StrTokenize which is a little
        expensive for instances in the hundreds.
        \param csvLine     : colon separated values
        \param field       : [out] second field
        \returns           True = Success, False = Fail
        */
        bool CSVSecondField(const std::wstring& csvLine, std::wstring & field);

        /*----------------------------------------------------------------------------*/
        /**
        Tokenize out the properties from the colon separated values of lpp listing.
        \param fileset     : colon separated values
        \param productID   : key identifier.  Used to search for install date in m_History.
        \param version     : from fileset
        \returns           True = Success, False = Fail
        */
        bool GetFilesetProperties(const std::wstring& fileset, std::wstring& description, std::wstring& version);

        /*----------------------------------------------------------------------------*/
        /**
        Tokenize out the properties from the colon separated values of lpp history.
        \param fileset     : colon separated values
        \param installDate : composed from install date and time.
        */
        bool GetFilesetProperties(const std::wstring& id, SCXCoreLib::SCXCalendarTime& installDate);

        /*----------------------------------------------------------------------------*/
        /**
        Read the lpp output and return all colon sep lines.
        \returns           True = Success, False = Fail
        */
        bool GetAllFilesetLines(void);

        /*----------------------------------------------------------------------------*/
        /**
        Argument to std::transform for lpp install time field.
        Example: '13;12;01' => '13:12:01'
        \returns ':' if arg is ';'. same char otherwise.
        */
        static int SemiToColon(int c) { return (c == (int)';' ? (int)':' : c); }

        typedef std::vector<std::wstring>         VECIDS;
        VECIDS               m_Ids;
        typedef std::map<std::wstring, std::wstring> MAPLPPLISTING;
        typedef std::map<std::wstring, std::wstring> MAPLPPHISTORY;
        MAPLPPLISTING        m_lppListing;
        MAPLPPHISTORY        m_lppHistory;

        /*----------------------------------------------------------------------------*/
        /**
         Fields from "lslpp -Lcq all"
         */
        typedef enum {
            LPPLISTPackageName,
            LPPLISTFileset,
            LPPLISTLevel,
            LPPLISTState,
            LPPLISTPTFId,
            LPPLISTFixState,
            LPPLISTType,
            LPPLISTDescription,
            LPPLISTDestinationDir,
            LPPLISTUninstaller,
            LPPLISTMessageCatalog,
            LPPLISTMessageSet,
            LPPLISTMessageNumber,
            LPPLISTParent,
            LPPLISTAutomatic,
            LPPLISTEFIXLocked,
            LPPLISTInstallPath,
            LPPLISTBuildDate,
            LPPLIST_MAX = LPPLISTBuildDate
        } LPPListingField;

        /*----------------------------------------------------------------------------*/
        /**
         Fields from "lslpp -hcq all"
         */
        typedef enum {
            LPPHISTPath,
            LPPHISTFileset,
            LPPHISTLevel,
            LPPHISTPTFId,
            LPPHISTAction,
            LPPHISTStatus,
            LPPHISTDate,
            LPPHISTTime,
            LPPHIST_MAX = LPPHISTTime
        } LPPHistoryField;
#endif

#if defined(linux)
    private:
        /*----------------------------------------------------------------------------*/
        /**
        Call RPM Query API
        \param argc : count of arguments of argv.
        \param argv : points to all arguments
        \returns     the result if the rpm command runs sucessfully,when invoking RPM API fails, it is errno. of failures ,otherwise 0.
        \throws SCXInternalErrorException if querying RPM fails
        */
        int InvokeRPMQuery(int argc, char * argv[]);
        /*----------------------------------------------------------------------------*/
        /**
        Call RPM Query API, and get the result
        \param argc : count of arguments of argv.
        \param argv : points to all arguments
        \param tempFileName : temporay file that the query result is saved to. 
        \param result : RPM API query result
        \throws SCXInternalErrorException if freopen stdout fails
        */
        void GetRPMQueryResult(int argc, char * argv[], const std::wstring& tempFileName, std::vector<std::wstring>& result);

#if defined(linux) && defined(PF_DISTRO_ULINUX)
        // helper struct used with GetDPKGTotal helper function
        struct PackageInfo
        {
            std::wstring Name;
            std::wstring Version;
            std::wstring Vendor;
            std::wstring Release;
            std::wstring BuildTime;
            std::wstring InstallTime;
            std::wstring BuildHost;
            std::wstring SourceRPM;
            std::wstring License;
            std::wstring Packager;
            std::wstring Group;
            std::wstring URL;
            std::wstring Summary;
        };
        
        /*----------------------------------------------------------------------------*/
        /**
        Gets information for searchedPackage in the map of installed DPKGs
        \param searchedPackage : the package to get info for 
        \param result : contains the information for this package, if found
        */
        void GetDPKGInfo(const std::wstring & searchedPackage, std::vector<std::wstring>& result);


        /*----------------------------------------------------------------------------*/
        /**
        Gets list of all installed DPKGs
        \param result : contains the list of all installed packages
        */
        void GetDPKGList(std::vector<std::wstring>& result);
        
        std::map<std::wstring, PackageInfo> DPKGMap; //!< contains the data of all installed DPKGs and infos
#endif // defined(PF_DISTRO_ULINUX)
#endif

    private:
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle.
        SCXCoreLib::SCXHandle<SoftwareDependencies> m_deps; //!< Used for dependency injection

    };

}

#endif /* INSTALLEDSOFTWAREDEPENDENCIES_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
