#pragma once

class ProxyResolver
{
    LOG_Context("ProxyResolver");

public:
    ProxyResolver();
    ~ProxyResolver();
    DWORD ResolveProxy(HINTERNET hSession, wchar const *pwszUrl);
    void ResetProxyCursor();
    void PrintProxyData();
    DWORD SetNextProxySetting(HINTERNET hInternet, DWORD dwRequestError);

private:
    bool m_fInit;                      // The proxy has been resolved.
    bool m_fReturnedFirstProxy;        // The first proxy in the list has been returned to the application.
    bool m_fReturnedLastProxy;         // The end of the proxy list was reached.
    WINHTTP_PROXY_INFO m_wpiProxyInfo; // The initial proxy and bypass list returned by calls to
                                       // WinHttpGetIEProxyConfigForCurrentUser and WinHttpGetProxyForUrl.
    bool m_fProxyFailOverValid;        // if it's is valid to iterate through the proxy list for proxy failover.
    wchar *m_pwszProxyCursor;          // The current location in the proxy list

    bool IsWhitespace(WCHAR wcChar);
    bool IsRecoverableAutoProxyError(DWORD dwError);
    bool IsErrorValidForProxyFailover(DWORD dwError);
    DWORD WinHttpGetProxyForUrlWrapper(HINTERNET hSession,
        wchar const *pwszUrl,
        WINHTTP_AUTOPROXY_OPTIONS *pwaoOptions,
        wchar **ppwszProxy,
        wchar **ppwszProxyBypass);
    DWORD GetProxyForAutoSettings(HINTERNET hSession,
        wchar const *pwszUrl,
        wchar const *pwszAutoConfigUrl,
        wchar **ppwszProxy,
        wchar **ppwszProxyBypass);
};

// m_fProxyFailOverValid - Proxy failover is valid for a list of proxies returned by executing a proxy script.  This
//                         occurs in auto-detect and auto-config URL proxy detection modes.  When static proxy settings
//                         are used fallback is not allowed.
