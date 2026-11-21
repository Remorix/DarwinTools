#include <CoreFoundation/CoreFoundation.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern CFDictionaryRef _CFCopySupplementalVersionDictionary(void);
extern const CFStringRef _kCFSystemVersionProductNameKey;
extern const CFStringRef _kCFSystemVersionProductVersionKey;
extern const CFStringRef _kCFSystemVersionProductVersionExtraKey;
extern const CFStringRef _kCFSystemVersionBuildVersionKey;

struct versionFields {
	char *productName;
	char *productVersion;
	char *productVersionExtra;
	char *buildVersion;
	char *releaseType;
	char *buildID;
};

enum outputField {
	OUTPUT_ALL,
	OUTPUT_PRODUCT_NAME,
	OUTPUT_PRODUCT_VERSION,
	OUTPUT_PRODUCT_VERSION_EXTRA,
	OUTPUT_BUILD_VERSION,
	OUTPUT_BUILD_ID,
	OUTPUT_XBS_VERSION,
	OUTPUT_RELEASE_TYPE,
};

static const struct option longOptions[] = {
	{"productName",         no_argument,       NULL, 'n'},
	{"ProductName",         no_argument,       NULL, 'n'},
	{"productname",         no_argument,       NULL, 'n'},
	{"productVersion",      no_argument,       NULL, 'v'},
	{"ProductVersion",      no_argument,       NULL, 'v'},
	{"productversion",      no_argument,       NULL, 'v'},
	{"productVersionExtra", no_argument,       NULL, 'e'},
	{"ProductVersionExtra", no_argument,       NULL, 'e'},
	{"productversionextra", no_argument,       NULL, 'e'},
	{"buildVersion",        no_argument,       NULL, 'b'},
	{"BuildVersion",        no_argument,       NULL, 'b'},
	{"buildversion",        no_argument,       NULL, 'b'},
	{"XBSVersion",          no_argument,       NULL, 'x'},
	{"xbsversion",          no_argument,       NULL, 'x'},
	{"releaseType",         no_argument,       NULL, 'r'},
	{"ReleaseType",         no_argument,       NULL, 'r'},
	{"releasetype",         no_argument,       NULL, 'r'},
	{"buildID",             no_argument,       NULL, 'i'},
	{"BuildID",             no_argument,       NULL, 'i'},
	{"buildid",             no_argument,       NULL, 'i'},
	{"plist",               required_argument, NULL, 'P'},
	{"help",                no_argument,       NULL, 'h'},
	{NULL,                  0,                 NULL,  0},
};

static const char usage[] =
	"Usage: sw_vers [--help|--productName|--productVersion|--productVersionExtra|--buildVersion]";

static const char unknownPlistError[] =
	"**** ERROR **** Unknown error loading system version plist: missing or malformed\n";

static bool loadVersionPlist(const char *path, CFPropertyListRef *plist, char **errorMessage);
static bool copyVersionFields(CFDictionaryRef dict, struct versionFields *fields);
static bool copyUTF8String(CFStringRef str, char **result);
static bool copyDictionaryString(CFDictionaryRef dict, CFStringRef key, char **result);

int main(int argc, char *argv[]) {
	enum outputField output = OUTPUT_ALL;
	char *path = NULL;
	int ch;

	while ((ch = getopt_long_only(argc, argv, "nvebxriP:h", longOptions, NULL)) != -1) {
		switch (ch) {
		case 'n':
			output = OUTPUT_PRODUCT_NAME;
			break;
		case 'v':
			output = OUTPUT_PRODUCT_VERSION;
			break;
		case 'e':
			output = OUTPUT_PRODUCT_VERSION_EXTRA;
			break;
		case 'b':
			output = OUTPUT_BUILD_VERSION;
			break;
		case 'i':
			output = OUTPUT_BUILD_ID;
			break;
		case 'x':
			output = OUTPUT_XBS_VERSION;
			break;
		case 'r':
			output = OUTPUT_RELEASE_TYPE;
			break;
		case 'P':
			path = strdup(optarg);
			break;
		case 'h':
			puts(usage);
			exit(0);
		default:
			puts(usage);
			exit(1);
		}
	}

	CFPropertyListRef dict = NULL;
	if (path == NULL) {
		dict = _CFCopySupplementalVersionDictionary();
	} else {
		char *errorMessage = NULL;
		if (!loadVersionPlist(path, &dict, &errorMessage)) {
			fprintf(stderr, "**** ERROR **** Unable to load system version from path '%s': %s\n",
				path, errorMessage);
			exit(1);
		}
	}

	if (dict == NULL) {
		fwrite(unknownPlistError, sizeof(unknownPlistError) - 1, 1, stderr);
		exit(1);
	}

	struct versionFields fields;
	copyVersionFields(dict, &fields);
	CFRelease(dict);

	switch (output) {
	case OUTPUT_PRODUCT_NAME:
		puts(fields.productName);
		break;
	case OUTPUT_PRODUCT_VERSION:
		puts(fields.productVersion);
		break;
	case OUTPUT_PRODUCT_VERSION_EXTRA:
		if (fields.productVersionExtra != NULL)
			puts(fields.productVersionExtra);
		break;
	case OUTPUT_BUILD_VERSION:
		puts(fields.buildVersion);
		break;
	case OUTPUT_BUILD_ID:
		if (fields.buildID != NULL)
			puts(fields.buildID);
		break;
	case OUTPUT_XBS_VERSION:
		break;
	case OUTPUT_RELEASE_TYPE:
		if (fields.releaseType != NULL)
			puts(fields.releaseType);
		break;
	case OUTPUT_ALL:
		printf("ProductName:\t\t%s\n", fields.productName);
		printf("ProductVersion:\t\t%s\n", fields.productVersion);
		if (fields.productVersionExtra != NULL)
			printf("ProductVersionExtra:\t%s\n", fields.productVersionExtra);
		printf("BuildVersion:\t\t%s\n", fields.buildVersion);
		if (fields.releaseType != NULL)
			printf("ReleaseType:\t\t%s\n", fields.releaseType);
		break;
	}

	exit(0);
}

static __attribute__((noinline)) bool
loadVersionPlist(const char *path, CFPropertyListRef *plist, char **errorMessage) {
	CFStringRef pathStr = CFStringCreateWithCString(kCFAllocatorDefault, path,
		kCFStringEncodingUTF8);
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, pathStr,
		kCFURLPOSIXPathStyle, false);
	CFDataRef data;

	// Keep the resource-loading API imported by both reference slices.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	Boolean loaded = CFURLCreateDataAndPropertiesFromResource(NULL, url, &data, NULL, NULL, NULL);
#pragma clang diagnostic pop
	if (!loaded) {
		*errorMessage = strdup("Could not load plist from disk");
		CFRelease(pathStr);
		CFRelease(url);
		return false;
	}

	CFErrorRef error = NULL;
	*plist = CFPropertyListCreateWithData(NULL, data, kCFPropertyListImmutable, NULL, &error);
	if (error != NULL) {
		CFStringRef description = CFErrorCopyDescription(error);
		if (!copyUTF8String(description, errorMessage))
			*errorMessage = strdup("Unknown error");
		CFRelease(description);
		CFRelease(error);
		CFRelease(pathStr);
		CFRelease(url);
		return false;
	}

	CFRelease(pathStr);
	CFRelease(url);
	CFRelease(data);
	return true;
}

static __attribute__((noinline)) bool
copyVersionFields(CFDictionaryRef dict, struct versionFields *fields) {
	copyDictionaryString(dict, _kCFSystemVersionProductNameKey, &fields->productName);
	copyDictionaryString(dict, _kCFSystemVersionProductVersionKey, &fields->productVersion);
	copyDictionaryString(dict, _kCFSystemVersionProductVersionExtraKey,
		&fields->productVersionExtra);
	copyDictionaryString(dict, _kCFSystemVersionBuildVersionKey, &fields->buildVersion);
	copyDictionaryString(dict, CFSTR("ReleaseType"), &fields->releaseType);
	return copyDictionaryString(dict, CFSTR("BuildID"), &fields->buildID);
}

static bool
copyUTF8String(CFStringRef str, char **result) {
	const char *cstr = CFStringGetCStringPtr(str, kCFStringEncodingUTF8);
	if (cstr != NULL) {
		*result = strdup(cstr);
		return true;
	}

	CFIndex byteCount;
	CFRange range = {0, CFStringGetLength(str)};
	CFStringGetBytes(str, range, kCFStringEncodingUTF8, 0, false, NULL, 0, &byteCount);
	*result = calloc(byteCount + 1, 1);
	range.length = CFStringGetLength(str);
	return CFStringGetBytes(str, range, kCFStringEncodingUTF8, 0, false,
		(UInt8 *)*result, byteCount, &byteCount) != 0;
}

static __attribute__((noinline)) bool
copyDictionaryString(CFDictionaryRef dict, CFStringRef key, char **result) {
	const void *value;
	*result = NULL;
	return CFDictionaryGetValueIfPresent(dict, key, &value) && copyUTF8String(value, result);
}
