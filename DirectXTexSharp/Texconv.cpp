#include "DirectXTexSharpLib.h"

#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>
#include <wincodec.h>



using namespace DirectXTexSharp;


//HRESULT __cdecl SaveToTGAMemory(_In_ const Image & image,
//	_In_ TGA_FLAGS flags,
//	_Out_ Blob & blob, _In_opt_ const TexMetadata * metadata = nullptr) noexcept;
void DirectXTexSharp::Texcconv::ConvertDdsImage(
	ScratchImage^ srcImage,
	System::String^ szFile,
	DirectXTexSharp::ESaveFileTypes filetype, 
    bool vflip, 
    bool hflip) {

	msclr::interop::marshal_context context;

    // defaults
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    DirectX::TEX_FILTER_FLAGS dwFilter = DirectX::TEX_FILTER_DEFAULT;
    DirectX::TEX_FILTER_FLAGS dwSRGB = DirectX::TEX_FILTER_DEFAULT;
    DirectX::TEX_FILTER_FLAGS dwConvert = DirectX::TEX_FILTER_DEFAULT;
    DirectX::TEX_COMPRESS_FLAGS dwCompress = DirectX::TEX_COMPRESS_DEFAULT;
    DirectX::TEX_FILTER_FLAGS dwFilterOpts = DirectX::TEX_FILTER_DEFAULT;
    float alphaThreshold = DirectX::TEX_THRESHOLD_DEFAULT;
    
    bool non4bc = false;



	auto metadata = srcImage->GetMetadata();
	auto info = *metadata->get_instance();
    auto imageptr = srcImage->get_instance();
    std::unique_ptr<DirectX::ScratchImage> image = (std::unique_ptr<DirectX::ScratchImage>)imageptr;

	// IsTypeless
	// IsPlanar
	// TODO


    //format = static_cast<DXGI_FORMAT>(LookupByName(pValue, g_pFormats));

    DXGI_FORMAT tformat = (format == DXGI_FORMAT_UNKNOWN) ? info.format : format;

	// decompress if needed
    //std::unique_ptr<ScratchImage> cimage;
    if (DirectX::IsCompressed(info.format))
    {
        // Direct3D can only create BC resources with multiple-of-4 top levels
        if ((info.width % 4) != 0 || (info.height % 4) != 0)
        {
            if (DirectX::IsCompressed(tformat))
            {
                non4bc = true;
            }
        }

        auto img = image->GetImage(0, 0, 0);
        assert(img);
        size_t nimg = image->GetImageCount();

        std::unique_ptr<DirectX::ScratchImage> timage(new (std::nothrow) DirectX::ScratchImage);
        if (!timage)
        {
            wprintf(L"\nERROR: Memory allocation failed\n");
            return;
        }

        auto hr = DirectX::Decompress(img, nimg, info, DXGI_FORMAT_UNKNOWN /* picks good default */, *timage);
        if (FAILED(hr))
        {
            wprintf(L" FAILED [decompress] (%08X%ls)\n", static_cast<unsigned int>(hr));
            return;
        }

        auto& tinfo = timage->GetMetadata();

        info.format = tinfo.format;

        assert(info.width == tinfo.width);
        assert(info.height == tinfo.height);
        assert(info.depth == tinfo.depth);
        assert(info.arraySize == tinfo.arraySize);
        assert(info.mipLevels == tinfo.mipLevels);
        assert(info.miscFlags == tinfo.miscFlags);
        assert(info.dimension == tinfo.dimension);

        image.swap(timage);
    }

    // --- Flip/Rotate -------------------------------------------------------------
    if (vflip | hflip)
    {
        std::unique_ptr<DirectX::ScratchImage> timage(new (std::nothrow) DirectX::ScratchImage);
        if (!timage)
        {
            wprintf(L"\nERROR: Memory allocation failed\n");
            return;
        }

        DirectX::TEX_FR_FLAGS dwFlags = DirectX::TEX_FR_ROTATE0;

        if (hflip)
            dwFlags |= DirectX::TEX_FR_FLIP_HORIZONTAL;

        if (vflip)
            dwFlags |= DirectX::TEX_FR_FLIP_VERTICAL;

        assert(dwFlags != 0);

        auto hr = FlipRotate(image->GetImages(), image->GetImageCount(), image->GetMetadata(), dwFlags, *timage);
        if (FAILED(hr))
        {
            wprintf(L" FAILED [fliprotate] (%08X%ls)\n", static_cast<unsigned int>(hr));
            return;
        }

        auto& tinfo = timage->GetMetadata();

        info.width = tinfo.width;
        info.height = tinfo.height;

        assert(info.depth == tinfo.depth);
        assert(info.arraySize == tinfo.arraySize);
        assert(info.mipLevels == tinfo.mipLevels);
        assert(info.miscFlags == tinfo.miscFlags);
        assert(info.format == tinfo.format);
        assert(info.dimension == tinfo.dimension);

        image.swap(timage);
        //cimage.reset();
    }

    // Resize
    // Swizzle
    // Color rotation
    // Tonemap
    
    // Convert
    // NormalMaps
    if (info.format != tformat && !DirectX::IsCompressed(tformat))
    {
        std::unique_ptr<DirectX::ScratchImage> timage(new (std::nothrow) DirectX::ScratchImage);
        if (!timage)
        {
            wprintf(L"\nERROR: Memory allocation failed\n");
            return;
        }

        auto hr = DirectX::Convert(image->GetImages(), image->GetImageCount(), image->GetMetadata(), tformat,
            dwFilter | dwFilterOpts | dwSRGB | dwConvert, alphaThreshold, *timage);
        if (FAILED(hr))
        {
            wprintf(L" FAILED [convert] (%08X%ls)\n", static_cast<unsigned int>(hr));
            return;
        }

        auto& tinfo = timage->GetMetadata();

        assert(tinfo.format == tformat);
        info.format = tinfo.format;

        assert(info.width == tinfo.width);
        assert(info.height == tinfo.height);
        assert(info.depth == tinfo.depth);
        assert(info.arraySize == tinfo.arraySize);
        assert(info.mipLevels == tinfo.mipLevels);
        assert(info.miscFlags == tinfo.miscFlags);
        assert(info.dimension == tinfo.dimension);

        image.swap(timage);
        //cimage.reset();
    }

    // ColorKey/ChromaKey /o
    // Invert Y Channel /o
    // Reconstruct Z Channel /o
    // Determine whether preserve alpha coverage is required /o
    // Generate mips
    
    // Preserve mipmap alpha coverage /o
    // Premultiplied alpha /o
    // Compress
    {
        //cimage.reset();
    }
    
    // Set alpha mode
    {
        if (DirectX::HasAlpha(info.format) && info.format != DXGI_FORMAT_A8_UNORM)
        {
            if (image->IsAlphaAllOpaque())
            {
                info.SetAlphaMode(DirectX::TEX_ALPHA_MODE_OPAQUE);
            }
            else if (info.IsPMAlpha())
            {
                // Aleady set TEX_ALPHA_MODE_PREMULTIPLIED
            }
            /*else if (dwOptions & (uint64_t(1) << OPT_SEPALPHA))
            {
                info.SetAlphaMode(TEX_ALPHA_MODE_CUSTOM);
            }*/
            else if (info.GetAlphaMode() == DirectX::TEX_ALPHA_MODE_UNKNOWN)
            {
                info.SetAlphaMode(DirectX::TEX_ALPHA_MODE_STRAIGHT);
            }
        }
        else
        {
            info.SetAlphaMode(DirectX::TEX_ALPHA_MODE_UNKNOWN);
        }
    }
    // Save result
    {
        auto img = image->GetImage(0, 0, 0);
        assert(img);
        size_t nimg = image->GetImageCount();

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        switch (filetype)
        {
        case DirectXTexSharp::ESaveFileTypes::TGA:
        {
            hr = DirectX::SaveToTGAFile(
                img[0], 
                DirectX::TGA_FLAGS_NONE, 
                context.marshal_as<const wchar_t*>(szFile), 
                /*(dwOptions & (uint64_t(1) << OPT_TGA20)) ? &info :*/ nullptr);
            break;
        }
        case DirectXTexSharp::ESaveFileTypes::HDR:
        {
            hr = DirectX::SaveToHDRFile(img[0], context.marshal_as<const wchar_t*>(szFile));
            break;
        }
        case DirectXTexSharp::ESaveFileTypes::JPEG:
        {
            DirectX::WICCodecs codec = DirectX::WICCodecs::WIC_CODEC_JPEG;
            size_t nimages = 1;

            hr = DirectX::SaveToWICFile(
                img, 
                nimages, 
                DirectX::WIC_FLAGS_NONE, 
                DirectX::GetWICCodec(codec), 
                context.marshal_as<const wchar_t*>(szFile), 
                nullptr, 
                GetWicPropsJpg);
            break;
        }
        case DirectXTexSharp::ESaveFileTypes::TIFF:
        {
            DirectX::WICCodecs codec = DirectX::WICCodecs::WIC_CODEC_TIFF;
            size_t nimages = 1;

            hr = DirectX::SaveToWICFile(
                img,
                nimages,
                DirectX::WIC_FLAGS_NONE,
                DirectX::GetWICCodec(codec),
                context.marshal_as<const wchar_t*>(szFile),
                nullptr,
                GetWicPropsTiff);
            break;
        }
        case DirectXTexSharp::ESaveFileTypes::PNG:
        {
            DirectX::WICCodecs codec = DirectX::WICCodecs::WIC_CODEC_TIFF;
            size_t nimages = 1;

            hr = DirectX::SaveToWICFile(
                img,
                nimages,
                DirectX::WIC_FLAGS_NONE,
                DirectX::GetWICCodec(codec),
                context.marshal_as<const wchar_t*>(szFile),
                nullptr,
                nullptr);
            break;
        }
        case DirectXTexSharp::ESaveFileTypes::BMP:
        {
            DirectX::WICCodecs codec = DirectX::WICCodecs::WIC_CODEC_TIFF;
            size_t nimages = 1;

            hr = DirectX::SaveToWICFile(
                img,
                nimages,
                DirectX::WIC_FLAGS_NONE,
                DirectX::GetWICCodec(codec),
                context.marshal_as<const wchar_t*>(szFile),
                nullptr,
                nullptr);
            break;
        }

        }

        if (FAILED(hr))
        {
            wprintf(L"Failed to initialize COM (%08X%ls)\n", static_cast<unsigned int>(hr));
            return;
        }
    }



}


void DirectXTexSharp::Texcconv::GetWicPropsJpg(IPropertyBag2* props)
{
    bool wicLossless = true;
    float wicQuality = -1.f;

    if (wicLossless || wicQuality >= 0.f)
    {
        PROPBAG2 options = {};
        VARIANT varValues = {};
        options.pstrName = const_cast<wchar_t*>(L"ImageQuality");
        varValues.vt = VT_R4;
        varValues.fltVal = (wicLossless) ? 1.f : wicQuality;
        (void)props->Write(1, &options, &varValues);
    }
}

void DirectXTexSharp::Texcconv::GetWicPropsTiff(IPropertyBag2* props)
{
    bool wicLossless = true;
    float wicQuality = -1.f;

    PROPBAG2 options = {};
    VARIANT varValues = {};
    if (wicLossless)
    {
        options.pstrName = const_cast<wchar_t*>(L"TiffCompressionMethod");
        varValues.vt = VT_UI1;
        varValues.bVal = WICTiffCompressionNone;
    }
    else if (wicQuality >= 0.f)
    {
        options.pstrName = const_cast<wchar_t*>(L"CompressionQuality");
        varValues.vt = VT_R4;
        varValues.fltVal = wicQuality;
    }
    (void)props->Write(1, &options, &varValues);
}