#include "DirectXTex.h"

#include "DirectXTexSharpLib.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>

#include "DirectXTexEnums.h"
#include "Image.h"
#include "ScratchImage.h"

using namespace DirectXTexSharp;


//---------------------------------------------------------------------------------
// Texture conversion, resizing, mipmap generation, and block compression

//HRESULT __cdecl Convert(
//	_In_ const Image& srcImage, _In_ DXGI_FORMAT format, _In_ TEX_FILTER_FLAGS filter, _In_ float threshold,
//	_Out_ ScratchImage& image) noexcept;
long DirectXTexSharp::Conversion::Convert(
	DirectXTexSharp::Image^ srcImage,
	DirectXTexSharp::DXGI_FORMAT_WRAPPED format,
	DirectXTexSharp::TEX_FILTER_FLAGS filter,
	const float threshold,
	DirectXTexSharp::ScratchImage^ image) {
	return DirectX::Convert(
		*srcImage->GetInstance(),
		static_cast<__dxgiformat_h__::DXGI_FORMAT> (format),
		static_cast<DirectX::TEX_FILTER_FLAGS> (filter),
		threshold,
		*image->GetInstance());

}

//HRESULT __cdecl Decompress(_In_ const Image& cImage, _In_ DXGI_FORMAT format, _Out_ ScratchImage& image) noexcept;
long DirectXTexSharp::Conversion::Decompress(
	Image^ cImage,
	DirectXTexSharp::DXGI_FORMAT_WRAPPED format,
	DirectXTexSharp::ScratchImage^ image) {
	return DirectX::Decompress(
		*cImage->GetInstance(),
		static_cast<__dxgiformat_h__::DXGI_FORMAT> (format),
		*image->GetInstance()
	);
};





