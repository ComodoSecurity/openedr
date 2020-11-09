//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_CATALOG_INFO Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust against a member of a Microsoft Catalog
//  file.
//
typedef struct WINTRUST_CATALOG_INFO_W8_
{
	DWORD           cbStruct;               // = sizeof(WINTRUST_CATALOG_INFO)

	DWORD           dwCatalogVersion;       // optional: Catalog version number
	LPCWSTR         pcwszCatalogFilePath;   // required: path/name to Catalog file

	LPCWSTR         pcwszMemberTag;         // optional: tag to member in Catalog
	LPCWSTR         pcwszMemberFilePath;    // required: path/name to member file
	HANDLE          hMemberFile;            // optional: open handle to pcwszMemberFilePath

	_Field_size_(cbCalculatedFileHash) BYTE            *pbCalculatedFileHash;  // optional: pass in the calculated hash
	DWORD           cbCalculatedFileHash;   // optional: pass in the count bytes of the calc hash

	PCCTL_CONTEXT   pcCatalogContext;       // optional: pass in to use instead of CatalogFilePath.

//#if (NTDDI_VERSION >= NTDDI_WIN8)
	HCATADMIN       hCatAdmin;              // optional for SHA-1 hashes, required for all other hash types.
//#endif // #if (NTDDI_VERSION >= NTDDI_WIN8)

} WINTRUST_CATALOG_INFO_W8, *PWINTRUST_CATALOG_INFO_W8;

//////////////////////////////////////////////////////////////////////////////
//
// WINTRUST_DATA Structure
//----------------------------------------------------------------------------
//  Used when calling WinVerifyTrust to pass necessary information into
//  the Providers.
//
typedef struct _WINTRUST_DATA_W8
{
    DWORD           cbStruct;                   // = sizeof(WINTRUST_DATA)

    LPVOID          pPolicyCallbackData;        // optional: used to pass data between the app and policy
    LPVOID          pSIPClientData;             // optional: used to pass data between the app and SIP.

    DWORD           dwUIChoice;                 // required: UI choice.  One of the following.
#                       define      WTD_UI_ALL              1
#                       define      WTD_UI_NONE             2
#                       define      WTD_UI_NOBAD            3
#                       define      WTD_UI_NOGOOD           4

    DWORD           fdwRevocationChecks;        // required: certificate revocation check options
#                       define      WTD_REVOKE_NONE         0x00000000
#                       define      WTD_REVOKE_WHOLECHAIN   0x00000001

    DWORD           dwUnionChoice;              // required: which structure is being passed in?
#                       define      WTD_CHOICE_FILE         1
#                       define      WTD_CHOICE_CATALOG      2
#                       define      WTD_CHOICE_BLOB         3
#                       define      WTD_CHOICE_SIGNER       4
#                       define      WTD_CHOICE_CERT         5
    union
    {
        struct WINTRUST_FILE_INFO_       *pFile;         // individual file
        struct WINTRUST_CATALOG_INFO_W8_ *pCatalog;      // member of a Catalog File
        struct WINTRUST_BLOB_INFO_       *pBlob;         // memory blob
        struct WINTRUST_SGNR_INFO_       *pSgnr;         // signer structure only
        struct WINTRUST_CERT_INFO_       *pCert;
    };

    DWORD           dwStateAction;                      // optional (Catalog File Processing)
#                       define      WTD_STATEACTION_IGNORE           0x00000000
#                       define      WTD_STATEACTION_VERIFY           0x00000001
#                       define      WTD_STATEACTION_CLOSE            0x00000002
#                       define      WTD_STATEACTION_AUTO_CACHE       0x00000003
#                       define      WTD_STATEACTION_AUTO_CACHE_FLUSH 0x00000004

    HANDLE          hWVTStateData;                      // optional (Catalog File Processing)

    WCHAR           *pwszURLReference;          // optional: (future) used to determine zone.

    DWORD           dwProvFlags;
#       define WTD_PROV_FLAGS_MASK                      0x0000FFFF
#       define WTD_USE_IE4_TRUST_FLAG                   0x00000001
#       define WTD_NO_IE4_CHAIN_FLAG                    0x00000002
#       define WTD_NO_POLICY_USAGE_FLAG                 0x00000004
#       define WTD_REVOCATION_CHECK_NONE                0x00000010
#       define WTD_REVOCATION_CHECK_END_CERT            0x00000020
#       define WTD_REVOCATION_CHECK_CHAIN               0x00000040
#       define WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT  0x00000080
#       define WTD_SAFER_FLAG                           0x00000100
#       define WTD_HASH_ONLY_FLAG                       0x00000200
#       define WTD_USE_DEFAULT_OSVER_CHECK              0x00000400
#       define WTD_LIFETIME_SIGNING_FLAG                0x00000800
#       define WTD_CACHE_ONLY_URL_RETRIEVAL             0x00001000 // affects CRL retrieval and AIA retrieval
#       define WTD_DISABLE_MD2_MD4                      0x00002000
#       define WTD_MOTW                                 0x00004000 // Mark-Of-The-Web
#       define WTD_CODE_INTEGRITY_DRIVER_MODE           0x00008000 // Code Integrity driver mode

    DWORD           dwUIContext;
#       define WTD_UICONTEXT_EXECUTE                    0
#       define WTD_UICONTEXT_INSTALL                    1

//#if (NTDDI_VERSION >= NTDDI_WIN8)
    struct WINTRUST_SIGNATURE_SETTINGS_    *pSignatureSettings;
//#endif // #if (NTDDI_VERSION >= NTDDI_WIN8)
} WINTRUST_DATA_W8, *PWINTRUST_DATA_W8;

//////////////////////////////////////////////////////////////////////////////
// WINTRUST_SIGNATURE_SETTINGS Structure
//----------------------------------------------------------------------------
//  Used to specify specific signatures on a file
//
//#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef struct WINTRUST_SIGNATURE_SETTINGS_
{
    DWORD cbStruct;             //sizeof(WINTRUST_SIGNATURE_SETTINGS)
    DWORD dwIndex;              //Index of the signature to validate
    DWORD dwFlags;              
    DWORD cSecondarySigs;       //A count of the secondary signatures
    DWORD dwVerifiedSigIndex;   //The index of the signature that was verified
    PCERT_STRONG_SIGN_PARA pCryptoPolicy; //Crypto policy the signature must pass
} WINTRUST_SIGNATURE_SETTINGS, *PWINTRUST_SIGNATURE_SETTINGS;

//Verifies the signature specified in WINTRUST_SIGNATURE_SETTINGS.dwIndex
#define WSS_VERIFY_SPECIFIC         0x00000001  
//Puts count of secondary signatures in WINTRUST_SIGNATURE_SETTINGS.cSecondarySigs
#define WSS_GET_SECONDARY_SIG_COUNT 0x00000002

//#endif // (NTDDI_VERSION >= NTDDI_WIN8)
