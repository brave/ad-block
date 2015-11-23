#include <string.h>
#include <math.h>
#include "filter.h"
#include "hashFn.h"
#include "ABPFilterParser.h"

#ifndef DISABLE_REGEX
#include <string>
#include <regex>
#endif

const char * getUrlHost(const char *input, int &len);

Filter::Filter() :
  borrowedData(false),
  filterType(FTNoFilterType),
  filterOption(FONoFilterOption),
  antiFilterOption(FONoFilterOption),
  data(nullptr),
  dataLen(-1),
  domainList(nullptr),
  host(nullptr) {
}

Filter::~Filter() {
  if (borrowedData) {
    return;
  }
  if (data) {
    delete[] data;
  }
  if (domainList) {
    delete[] domainList;
  }
  if (host) {
    delete[] host;
  }
}

Filter::Filter(const Filter &other) {
  borrowedData = other.borrowedData;
  filterType = other.filterType;
  filterOption = other.filterOption;
  antiFilterOption = other.antiFilterOption;
  dataLen = other.dataLen;
  if (other.dataLen == -1 && other.data) {
    dataLen = strlen(other.data);
  }

  if (other.borrowedData) {
    data = other.data;
    domainList = other.domainList;
    host = other.host;
  } else {
    if (other.data) {
      data = new char[dataLen];
      memcpy(data, other.data, dataLen);
    } else {
      data = nullptr;
    }
    if (other.domainList) {
       domainList = new char[strlen(other.domainList) + 1];
       strcpy(domainList, other.domainList);
    } else {
      domainList = nullptr;
    }
    if (other.host) {
      host = new char[strlen(other.host) + 1];
      strcpy(host, other.host);
    } else {
      host = nullptr;
    }
  }
}

void Filter::swap(Filter &other) {
  FilterType tempFilterType = filterType;
  FilterOption tempFilterOption = filterOption;
  FilterOption tempAntiFilterOption = antiFilterOption;
  char *tempData = data;
  int tempDataLen = dataLen;
  char *tempDomainList = domainList;
  char *tempHost = host;

  filterType = other.filterType;
  filterOption = other.filterOption;
  antiFilterOption = other.antiFilterOption;
  data = other.data;
  dataLen = other.dataLen;
  domainList = other.domainList;
  host = other.host;

  other.filterType = tempFilterType;
  other.filterOption = tempFilterOption;
  other.antiFilterOption = tempAntiFilterOption;
  other.data = tempData;
  other.dataLen = tempDataLen;
  other.domainList = tempDomainList;
  other.host = tempHost;
}

bool isDomain(const char *input, int len, const char *domain, bool anti) {
  const char *p = input;
  if (anti) {
    if (len >= 1 && p[0] != '~') {
      return false;
    } else {
      len--;
      p++;
    }
  }
  return !memcmp(p, domain, len);
}

bool Filter::containsDomain(const char *domain, bool anti) const {
  if (!domainList) {
    return false;
  }

  int startOffset = 0;
  int len = 0;
  const char *p = domainList;
  while (*p != '\0') {
    if (*p == '|') {
      if (isDomain(domainList + startOffset, len, domain, anti)) {
        return true;
      }
      startOffset += len + 1;
      len = -1;
    }
    p++;
    len++;
  }
  return isDomain(domainList + startOffset, len, domain, anti);
}

int Filter::getDomainCount(bool anti) const {
  if (!domainList || domainList[0] == '\0') {
    return 0;
  }

  int count = 0;
  int startOffset = 0;
  int len = 0;
  const char *p = domainList;
  while (*p != '\0') {
    if (*p == '|') {
      if (*(domainList + startOffset) == '~' && anti) {
        count++;
      }
      else if (*(domainList + startOffset) != '~' && !anti) {
        count++;
      }
      startOffset = len + 1;
      len = -1;
    }
    p++;
    len++;
  }

  if (*(domainList + startOffset) == '~' && anti) {
    count++;
  }
  else if (*(domainList + startOffset) != '~' && !anti) {
    count++;
  }
  return count;
}

void Filter::parseOption(const char *input, int len) {
  FilterOption *pFilterOption = &filterOption;
  const char *pStart = input;
  if (input[0] == '~') {
    pFilterOption = &antiFilterOption;
    pStart++;
    len --;
  }

  if (len >= 7 && !strncmp(pStart, "domain=", 7)) {
    len -= 7;
    domainList = new char[len + 1];
    domainList[len] = '\0';
    memcpy(domainList, pStart + 7, len);
  } else if (!strncmp(pStart, "script", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOScript);
  } else if (!strncmp(pStart, "image", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOImage);
  } else if (!strncmp(pStart, "stylesheet", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOStylesheet);
  } else if (!strncmp(pStart, "object", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOObject);
  } else if (!strncmp(pStart, "xmlhttprequest", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOXmlHttpRequest);
  } else if (!strncmp(pStart, "object-subrequest", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOObjectSubrequest);
  } else if (!strncmp(pStart, "subdocument", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOSubdocument);
  } else if (!strncmp(pStart, "document", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FODocument);
  } else if (!strncmp(pStart, "xbl", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOXBL);
  } else if (!strncmp(pStart, "collapse", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOCollapse);
  } else if (!strncmp(pStart, "donottrack", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FODoNotTrack);
  } else if (!strncmp(pStart, "other", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOOther);
  } else if (!strncmp(pStart, "elemhide", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOElemHide);
  } else if (!strncmp(pStart, "third-party", len)) {
    *pFilterOption = static_cast<FilterOption>(*pFilterOption | FOThirdParty);
  }
  // Otherwise just ignore the option, maybe something new we don't support yet
}

void Filter::parseOptions(const char *input) {
  filterOption = FONoFilterOption;
  antiFilterOption = FONoFilterOption;
  int startOffset = 0;
  int len = 0;
  const char *p = input;
  while (*p != '\0' && *p != '\n') {
    if (*p == ',') {
      parseOption(input + startOffset, len);
      startOffset += len + 1;
      len = -1;
    }
    p++;
    len++;
  }
  parseOption(input + startOffset, len);
}

bool endsWith(const char *input, const char *sub, int inputLen, int subLen) {
  if (subLen > inputLen) {
    return false;
  }

  int startCheckPos = inputLen - subLen;
  const char *p = input + startCheckPos;
  const char *q = sub;
  while (q != sub + subLen) {
    if (*(p++) != *(q++)) {
      return false;
    }
  }
  return true;
}

bool isThirdPartyHost(const char *baseContextHost, int baseContextHostLen, const char *testHost, int testHostLen) {
  if (!endsWith(testHost, baseContextHost, testHostLen, baseContextHostLen)) {
    return true;
  }

  // baseContextHost matches testHost exactly
  if (testHostLen == baseContextHostLen) {
    return false;
  }

  char c = testHost[testHostLen - baseContextHostLen - 1];
  return c != '.' && testHostLen != baseContextHostLen;
}

// Determines if there's a match based on the options, this doesn't
// mean that the filter rule should be accepted, just that the filter rule
// should be considered given the current context.
// By specifying context params, you can filter out the number of rules which are
// considered.
bool Filter::matchesOptions(const char *input, FilterOption context, const char *contextDomain) {
  // Maybe the user of the library can't determine a context because they're
  // blocking a the HTTP level, don't block here because we don't have enough
  // information
  if (context != FONoFilterOption) {
    if ((filterOption & ~FOThirdParty) != FONoFilterOption && !(filterOption & context)) {
      return false;
    }

    if ((antiFilterOption & ~FOThirdParty) != FONoFilterOption && (antiFilterOption & context)) {
      return false;
    }
  }

  // Domain options check
  if (domainList && contextDomain) {
    // + 2 because we always end in a |\0 for these buffers
    int bufSize = strlen(domainList) + 2;
    char *shouldBlockDomains = new char[bufSize];
    char *shouldSkipDomains = new char[bufSize];
    memset(shouldBlockDomains, 0, bufSize);
    memset(shouldSkipDomains, 0, bufSize);
    filterDomainList(domainList, shouldBlockDomains, contextDomain, false);
    filterDomainList(domainList, shouldSkipDomains, contextDomain, true);

    int leftOverBlocking = getLeftoverDomainCount(shouldBlockDomains, shouldSkipDomains);
    int leftOverSkipping = getLeftoverDomainCount(shouldSkipDomains, shouldBlockDomains);
    int shouldBlockDomainsLen = strlen(shouldBlockDomains);
    int shouldSkipDomainsLen = strlen(shouldSkipDomains);

    delete[] shouldBlockDomains;
    delete[] shouldSkipDomains;

    if ((shouldBlockDomainsLen == 0 && getDomainCount() != 0) ||
        (shouldBlockDomainsLen > 0 && leftOverBlocking == 0) ||
        (shouldSkipDomainsLen > 0 && leftOverSkipping > 0)) {
      return false;
    }
  }

  // If we're in the context of third-party site, then consider third-party option checks
  if (context & (FOThirdParty | FONotThirdParty)) {
    if ((filterOption & FOThirdParty) && host) {
      int inputHostLen;
      const char *inputHost = getUrlHost(input, inputHostLen);
      bool inputHostIsThirdParty = isThirdPartyHost(host, strlen(host), inputHost, inputHostLen);
      if (inputHostIsThirdParty || !(context & FOThirdParty)) {
        return false;
      }
    }
  }

  return true;
}


const char * getNextPos(const char *input, char separator, const char *end) {
  const char *p = input;
  while (p != end && *p != '\0' && *p != separator) {
    p++;
  }
  return p;
}

int indexOf(const char *source, const char *filterPartStart, const char *filterPartEnd) {
  const char *s = source;
  const char *fStart = filterPartStart;
  const char *notCheckedSource = source;

  while (*s != '\0') {
    if (fStart == filterPartEnd) {
      return s - source - (filterPartEnd - filterPartStart);
    }
    if (*s != *fStart) {
      notCheckedSource++;
      s = notCheckedSource;
      fStart = filterPartStart;
      continue;
    }

    fStart++;
    s++;
  }

  if (fStart == filterPartEnd) {
    return s - source - (filterPartEnd - filterPartStart);
  }

  return -1;
}

/**
 * Similar to str1.indexOf(filter, startingPos) but with
 * extra consideration to some ABP filter rules like ^.
 */
int indexOfFilter(const char* input, int inputLen, const char *filterPosStart, const char *filterPosEnd) {
  bool prefixedSeparatorChar = false;
  int filterLen = filterPosEnd - filterPosStart;
  int index = 0;
  int beginIndex = -1;
  if (filterLen > inputLen) {
    return -1;
  }

  const char *filterPartStart = filterPosStart;
  const char *filterPartEnd = getNextPos(filterPosStart, '^', filterPosEnd);
  if (filterPartEnd - filterPosEnd > 0) {
    filterPartEnd = filterPosEnd;
  }

  while (*(input + index) != '\0') {
    if (filterPartStart == filterPartEnd && filterPartStart != filterPosStart) {
      prefixedSeparatorChar = true;
    }
    int lastIndex = index;
    index = indexOf(input + index, filterPartStart, filterPartEnd);
    if (index == -1) {
      return -1;
    }
    index += lastIndex;
    if (beginIndex == -1) {
      beginIndex = index;
    }

    index += (filterPartEnd - filterPartStart);

    if (prefixedSeparatorChar) {
      char testChar = *(input + index + (filterPartEnd - filterPartStart));
      if (!isSeparatorChar(testChar)) {
        return -1;
      }
    }

    if (filterPartEnd == filterPosEnd || *filterPartEnd == '\0') {
      break;
    }
    const char *temp = getNextPos(filterPartEnd + 1, '^', filterPosEnd);
    filterPartStart = filterPartEnd + 1;
    filterPartEnd = temp;
    prefixedSeparatorChar = false;
    if (filterPartEnd - filterPosEnd > 0) {
      break;
    }
  }

  return beginIndex;
}

bool Filter::matches(const char *input, FilterOption contextOption, const char *contextDomain, BloomFilter *inputBloomFilter) {
  return matches(input, strlen(input), contextOption, contextDomain, inputBloomFilter);
}

bool Filter::matches(const char *input, int inputLen, FilterOption contextOption, const char *contextDomain, BloomFilter *inputBloomFilter) {

  if (!matchesOptions(input, contextOption, contextDomain)) {
    return false;
  }

  if (!data) {
    return false;
  }

  // We lazily figure out the dataLen only once
  if (dataLen == -1) {
    dataLen = strlen(data);
  }

  // Check for a regex match
  if (filterType & FTRegex) {
#ifndef DISABLE_REGEX
    std::smatch m;
    std::regex e (data, std::regex_constants::extended);
    return std::regex_search(std::string(input), m, e);
#else
    return false;
#endif
  }

  // Check for both left and right anchored
  if ((filterType & FTLeftAnchored) && (filterType & FTRightAnchored)) {
    return !strcmp(data, input);
  }

  // Check for right anchored
  if (filterType & FTRightAnchored) {
    if (dataLen > inputLen) {
      return false;
    }

    return !strcmp(input + (inputLen - dataLen), data);
  }

  // Check for left anchored
  if (filterType & FTLeftAnchored) {
    return !strncmp(data, input, dataLen);
  }

  // Check for domain name anchored
  if (filterType & FTHostAnchored) {

    const char *filterPartEnd = data + dataLen;
    int currentHostLen;
    const char *currentHost = getUrlHost(input, currentHostLen);
    int hostLen = strlen(host);

    if (inputBloomFilter) {
      for (int i = 1; i < hostLen; i++) {
        if (!inputBloomFilter->exists(host + i - 1, 2)) {
          return false;
        }
      }
    }

    return !isThirdPartyHost(host, hostLen, currentHost, currentHostLen) &&
      indexOfFilter(input, inputLen, data, filterPartEnd) != -1;
  }

  // Wildcard match comparison
  const char *filterPartStart = data;
  const char *filterPartEnd = getNextPos(data, '*', data + dataLen);
  int index = 0;
  while (filterPartStart != filterPartEnd) {
    int filterPartLen = filterPartEnd - filterPartStart;

    if (inputBloomFilter) {
      for (int i = 1; i < filterPartLen && filterPartEnd - filterPartStart - i >= 2; i++) {
        if (!inputBloomFilter->exists(filterPartStart + i - 1, 2)) {
          return false;
        }
      }
    }

    int newIndex = indexOfFilter(input + index, inputLen - index, filterPartStart, filterPartEnd);
    if (newIndex == -1) {
      return false;
    }
    newIndex += index;

    if (*filterPartEnd == '\0') {
      break;
    }
    const char *temp = getNextPos(filterPartEnd + 1, '*', data + dataLen);
    filterPartStart = filterPartEnd + 1;
    filterPartEnd = temp;
    index = newIndex + filterPartLen;
    if (*(input + newIndex) == '\0') {
      break;
    }
  }

  return true;
}

void Filter::filterDomainList(const char *domainList, char *destBuffer, const char *contextDomain, bool anti) {
  if (!domainList) {
    return;
  }

  char *curDest = destBuffer;
  int contextDomainLen = strlen(contextDomain);
  int startOffset = 0;
  int len = 0;
  const char *p = domainList;
  while (true) {
    if (*p == '|' || *p == '\0') {

      const char *domain = domainList + startOffset;
      if (!isThirdPartyHost(domain[0] == '~' ? domain + 1 : domain, domain[0] == '~' ? len -1 : len, contextDomain, contextDomainLen)) {
        // We're only considering domains, not anti domains
        if (!anti && len > 0 && *domain != '~') {
          memcpy(curDest, domain, len);
          curDest[len] = '|';
          curDest[len + 1] = '\0';
        } else if (anti && len > 0 && *domain == '~') {
          memcpy(curDest, domain + 1, len - 1);
          curDest[len] = '|';
          curDest[len + 1] = '\0';
        }
      }

      startOffset += len + 1;
      len = -1;
    }

    if (*p == '\0') {
      break;
    }
    p++;
    len++;
  }
}

bool isEveryDomainThirdParty(const char *shouldSkipDomains, const char *shouldBlockDomain, int shouldBlockDomainLen) {
  bool everyDomainThirdParty = true;
  if (!shouldSkipDomains) {
    return false;
  }

  int startOffset = 0;
  int len = 0;
  const char *p = shouldSkipDomains;
  while (true) {
    if (*p == '|' || *p == '\0') {

      const char *domain = shouldSkipDomains + startOffset;
      if (*domain == '~') {
        everyDomainThirdParty = everyDomainThirdParty &&
          isThirdPartyHost(shouldBlockDomain, shouldBlockDomainLen, domain + 1, len - 1);
      } else {
        everyDomainThirdParty = everyDomainThirdParty &&
          isThirdPartyHost(shouldBlockDomain, shouldBlockDomainLen, domain, len);
      }

      startOffset += len + 1;
      len = -1;
    }

    if (*p == '\0') {
      break;
    }
    p++;
    len++;
  }

  return everyDomainThirdParty;
}

int Filter::getLeftoverDomainCount(const char *shouldBlockDomains, const char *shouldSkipDomains) {
  int leftOverBlocking = 0;

  if (strlen(shouldBlockDomains) == 0) {
    return 0;
  }

  int startOffset = 0;
  int len = 0;
  const char *p = domainList;
  while (true) {
    if (*p == '|' || *p == '\0') {
      const char *domain = domainList + startOffset;
      if (*domain == '~') {
        if (isEveryDomainThirdParty(shouldSkipDomains, domain + 1, len - 1)) {
          leftOverBlocking++;
        }
      } else {
        if (isEveryDomainThirdParty(shouldSkipDomains, domain, len)) {
          leftOverBlocking++;
        }
      }

      startOffset += len + 1;
      len = -1;
    }

    if (*p == '\0') {
      break;
    }
    p++;
    len++;
  }

  return leftOverBlocking;
}

uint64_t Filter::hash() const {
  HashFn fn(19);
  if (!host && !data) {
    return 0;
  } else if (host) {
    return fn(host, strlen(host));
  }

  return fn(data, dataLen);
}

uint32_t Filter::serialize(char *buffer) {
  uint32_t totalSize = 0;
  char sz[64];
  uint32_t dataLenSize = 1 + sprintf(sz, "%x,%x,%x,%x", dataLen, filterType, filterOption, antiFilterOption);
  if (buffer) {
    memcpy(buffer + totalSize, sz, dataLenSize);
  }
  totalSize += dataLenSize;
  if (buffer) {
    memcpy(buffer + totalSize, data, dataLen);
  }
  totalSize += dataLen;

  if (host) {
    int hostLen = strlen(host);
    if (buffer) {
      memcpy(buffer + totalSize, host, hostLen + 1);
    }
    totalSize += hostLen;
  }
  totalSize += 1;

  if (domainList) {
    int domainListLen = strlen(domainList);
    if (buffer) {
      memcpy(buffer + totalSize, domainList, domainListLen + 1);
    }
    totalSize += domainListLen;
  }
  totalSize += 1;

  return totalSize;
}

bool hasNewlineBefore(char *buffer, uint32_t bufferSize) {
  char *p = buffer;
  for (uint32_t i = 0; i < bufferSize; ++i) {
    if (*p == '\0')
      return true;
    p++;
  }
  return false;
}

uint32_t Filter::deserialize(char *buffer, uint32_t bufferSize) {
  dataLen = 0;
  if (!hasNewlineBefore(buffer, bufferSize)) {
    return 0;
  }
  sscanf(buffer, "%x,%x,%x,%x", &dataLen, &filterType, &filterOption, &antiFilterOption);
  uint32_t consumed = strlen(buffer) + 1;
  if (consumed + dataLen >= bufferSize) {
    return 0;
  }

  data = buffer + consumed;
  consumed += dataLen;

  uint32_t hostLen = strlen(buffer + consumed);
  if (hostLen != 0) {
    host = buffer + consumed;
  } else {
    host = nullptr;
  }
  consumed += hostLen + 1;

  uint32_t domainListLen = strlen(buffer + consumed);
  if (domainListLen != 0) {
    domainList = buffer + consumed;
  } else {
    domainList = nullptr;
  }
  consumed += domainListLen + 1;

  borrowedData = true;

  return consumed;
}

