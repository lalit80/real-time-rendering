#include <stdio.h>
#include <math.h>
#include <d3d11.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

int main(void) {
    IDXGIFactory* pIDXGIFactory = NULL;
    IDXGIAdapter* pIDXGIAdapter = NULL;
    DXGI_ADAPTER_DESC dxgiAdapterDesc;
    HRESULT hr = S_OK;
    char str[256];

    hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pIDXGIFactory);
    if (FAILED(hr)) {
        printf("CreateDXGIFactory failed. hr = 0x%08x\n", hr);
        goto cleanup;
    }
    
    if (pIDXGIFactory->EnumAdapters(0, &pIDXGIAdapter) != DXGI_ERROR_NOT_FOUND) {
        ZeroMemory(&dxgiAdapterDesc, sizeof(DXGI_ADAPTER_DESC));
        pIDXGIAdapter->GetDesc(&dxgiAdapterDesc);

        // to convert wchar type into char type
        WideCharToMultiByte(CP_ACP, 0, dxgiAdapterDesc.Description, 256, str, 256, NULL, NULL);
        printf("Graphic Device Name = %s\n", str);
        printf("VRAM(in bytes) = %I64d\n", (__int64)dxgiAdapterDesc.DedicatedVideoMemory);
        printf("VRAM(in GB) = %d\n", (int)ceil((double)dxgiAdapterDesc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0)));
    } else {
        printf("IDXGIFactory::EnumAdapters failed.\n");
        goto cleanup;
    }
    
cleanup:
    if (pIDXGIAdapter) {
        pIDXGIAdapter->Release();
        pIDXGIAdapter = NULL;
    }
    if (pIDXGIFactory) {
        pIDXGIFactory->Release();
        pIDXGIFactory = NULL;
    }

    return 0;
}
