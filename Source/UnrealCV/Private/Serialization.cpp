#include "UnrealCVPrivate.h"
#include "Serialization.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "cnpy.h"
#include <chrono>  // for high_resolution_clock

TArray<uint8> SerializationUtils::Array2Npy(const TArray<FFloat16Color>& ImageData, int32 Width, int32 Height, int32 Channel)
{
	// Record start time
	auto start = std::chrono::high_resolution_clock::now();

	float *TypePointer = nullptr; // Only used for determing the type

	std::vector<int> Shape;
	Shape.push_back(Height);
	Shape.push_back(Width);
	if (Channel != 1) Shape.push_back(Channel);

	std::vector<char> NpyHeader = cnpy::create_npy_header(TypePointer, Shape);

	// Append the actual data
	// FIXME: A slow implementation to convert TArray<FFloat16Color> to binary.
	/* A small size test
	std::vector<char> NpyData;
	for (int i = 0; i < 3 * 3 * 3; i++)
	{
		NpyData.push_back(i);
	}
	*/
	// std::vector<char> NpyData;
	std::vector<float> FloatData;
	float DebugMin = 10e10, DebugMax = 0.0;

	for (int i = 0; i < ImageData.Num(); i++)
	{
		if (Channel == 1)
		{
			float v = ImageData[i].R;
			FloatData.push_back(ImageData[i].R);
			// debug: Check the range of data
			if (v < DebugMin) DebugMin = v;
			if (v > DebugMax) DebugMax = v;
		}
		if (Channel == 3)
		{
			FloatData.push_back(ImageData[i].R);
			FloatData.push_back(ImageData[i].G);
			FloatData.push_back(ImageData[i].B); // TODO: Is this a correct order in numpy?
		}
	}
	check(FloatData.size() == Width * Height * Channel);
	// Convert to binary array
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&FloatData[0]);

	// https://stackoverflow.com/questions/22629728/what-is-the-difference-between-char-and-unsigned-char
	// https://stackoverflow.com/questions/11022099/convert-float-vector-to-byte-vector-and-back
	std::vector<unsigned char> NpyData(bytes, bytes + sizeof(float) * FloatData.size());

	NpyHeader.insert(NpyHeader.end(), NpyData.begin(), NpyData.end());

	// FIXME: Find a more efficient implementation
	TArray<uint8> BinaryData;
	for (char Element : NpyHeader)
	{
		BinaryData.Add(Element);
	}

	// Record end time
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	UE_LOG(LogUnrealCV, Error, TEXT("Array2Npy elapsed time: %fs"), elapsed.count());

	return BinaryData;
}

TArray<uint8> SerializationUtils::Image2Png(const TArray<FColor>& Image, int Width, int Height)
{
	// Record start time
	auto start = std::chrono::high_resolution_clock::now();

	if (Image.Num() == 0 || Image.Num() != Width * Height)
	{
		return TArray<uint8>();
	}
	static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	static TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	ImageWrapper->SetRaw(Image.GetData(), Image.GetAllocatedSize(), Width, Height, ERGBFormat::BGRA, 8);
	const TArray<uint8>& ImgData = ImageWrapper->GetCompressed((int32)EImageCompressionQuality::Uncompressed);

	// Record end time
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	UE_LOG(LogUnrealCV, Error, TEXT("Image2Png elapsed time: %fs"), elapsed.count());

	return ImgData;
}

TArray<uint8> SerializationUtils::Image2Exr(const TArray<FFloat16Color>& FloatImage, int Width, int Height)
{
	if (FloatImage.Num() == 0 || FloatImage.Num() != Width * Height)
	{
		return TArray<uint8>();
	}
	static IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	static TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::EXR);
	ImageWrapper->SetRaw(FloatImage.GetData(), FloatImage.GetAllocatedSize(), Width, Height, ERGBFormat::RGBA, 16);
	const TArray<uint8>& ExrData = ImageWrapper->GetCompressed((int32)EImageCompressionQuality::Uncompressed);
	return ExrData;
}
