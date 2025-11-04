#ifndef MeshCodecFloatCodec_h
#define MeshCodecFloatCodec_h

#include "iGameMacro.h"
#include "iGameThreadPool.h"
#include "iGameMeshCodecParamSet.h"
#include <functional>

IGAME_NAMESPACE_BEGIN

// 经过测试zfp的压缩率远不如meshopt
class MeshOptFloatCodec {
public:
	template<typename T>
	static void FloatEncoder(
		std::vector<unsigned char>& dest,
		std::vector<T>& source, // float* double*
		const FloatParameters& floatParams,
		const FloatErrorControlParameters& errorParams
		)
	{
		IGsize elementCount = floatParams.elementCount;
		int dimension = floatParams.dimension;
		IGsize valueCount = elementCount * dimension;

		// void* -> vector<float> / vector<double>
		// 定点数化
		int encodeVertexSize = floatParams.valueSize * dimension;
		int encodeElementCount = elementCount;


		float max_val = source[0];
		float min_val = source[0];


		auto Quantize = [&](float value, int bits) -> float {
			if (value == 0.0f) {
				return 0.0f;
			}

			uint32_t max_quantized_value = (1U << bits) - 1;
			float clipped_value = std::max(min_val, std::min(value, max_val));
			float normalized_value = (clipped_value - min_val) / (max_val - min_val);
			uint32_t quantized_value = static_cast<uint32_t>(
				std::round(normalized_value * max_quantized_value)
				);
			float normalized_dequantized = static_cast<float>(quantized_value) / max_quantized_value;
			float dequantized_value = normalized_dequantized * (max_val - min_val) + min_val;
			return dequantized_value;
			};

		auto StrengthToBits = [=](float strength, int minBits, int maxBits) -> int {
			strength = std::min(std::max(strength, 0.0f), 1.0f);
			const int levels = maxBits - minBits + 1;
			const int index = (strength == 0.0f) ? 0 : static_cast<int>(strength * levels);
			const int cappedIndex = std::min(index, levels - 1);
			return maxBits - cappedIndex;
			};

		auto QuantizeStrengthToBits = [=](float strength) -> int {
			// 损坏最高 损坏最低
				return StrengthToBits(strength, 8, 32);
			};

		auto MantissaStrengthToBits = [=](float strength) -> int {

			return StrengthToBits(strength, 4, 23);
			};

		auto QuantizeErrorToBits = [&](double epsilon) -> int {
			float minValue = *std::min_element(source.begin(), source.end());
			float maxValue = *std::max_element(source.begin(), source.end());

			// 处理无效参数
			if (minValue <= 0 || maxValue <= 0 || epsilon <= 0 || maxValue < minValue) {
				return -1; // 返回错误值
			}

			// 计算所需位数N
			double logRange = log(maxValue / minValue);
			double requiredBits = log2(logRange / epsilon + 1);

			// 向上取整，确保满足误差要求
			return static_cast<int>(ceil(requiredBits));
			};

		auto MantissaErrorToBits = [&](double epsilon) -> int {
			if (epsilon <= 0 || epsilon >= 1)
				return -1; // 无效的误差值

			return static_cast<int>(ceil(log2(1.0 / epsilon) - 1));
			};

		auto doQuantize = [&](std::function<float(float value, int bits)> quantFunc, std::function<int(float error)> errorFunc) -> void
			{
				switch (floatParams.errorMode)
				{
				case ErrorMode::Default:
				{
					int bits = errorFunc(floatParams.defaultErrorBound);
					
					ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
						for (int i = start; i < end; i++) {
							source[i] = quantFunc(source[i], bits);
						}
						});

					break;
				}
				case ErrorMode::KeyArea:
				{
					int keyBits = errorFunc(floatParams.keyAreaErrorBound);
					int nonKeyBits = errorFunc(floatParams.nonKeyAreaErrorBound);

					ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
						for (int i = start; i < end; i++) {
							if (errorParams.isKeyElement[i / dimension])
							{
								source[i] = quantFunc(source[i], keyBits);
							}
							else
							{
								source[i] = quantFunc(source[i], nonKeyBits);
							}
						}
						});

					break;
				}
				default:
					break;
				}
			};

		if (floatParams.errorMode != ErrorMode::None)
		{
			switch (floatParams.lossyMode)
			{
			case LossyMode::Quantization:
			{
				for (const auto& val : source) {
					if (val > max_val) {
						max_val = val;
					}
					if (val < min_val) {
						min_val = val;
					}
				}

				doQuantize(Quantize, QuantizeStrengthToBits);
				break;
			}
			case LossyMode::MantissaTruncation:
			{
				doQuantize(meshopt_quantizeFloat, MantissaStrengthToBits);
				break;
			}
			default:
				break;
			}
		}

		// 编码
		dest.resize(meshopt_encodeVertexBufferBound(encodeElementCount, encodeVertexSize));
		dest.resize(meshopt_encodeVertexBuffer(dest.data(), dest.size(), source.data(),
			encodeElementCount, encodeVertexSize));

		return;
	}

	// dest需要在外部先开辟好空间
	template<typename T>
	static void FloatDecoder(std::vector<T>& dest,
		const std::vector<unsigned char>& source,
		const FloatParameters& floatParams)
	{
		IGsize valueCount = floatParams.elementCount * floatParams.dimension;
		dest.resize(valueCount);

		IGsize vertexSize = floatParams.valueSize * floatParams.dimension;
		meshopt_decodeVertexBuffer(
			dest.data(),
			floatParams.elementCount,
			vertexSize,
			source.data(),
			source.size()
		);

		return;
	}
};

class FloatCodecError {
public:
	static void EachElementError(
		const std::vector<float>& source,
		const std::vector<float>& quantized,
		const FloatParameters& floatParams,
		std::vector<float>& relError,
		std::vector<float>& absError)
	{
		// 调整误差数组大小为元素数量
		relError.resize(floatParams.elementCount);
		absError.resize(floatParams.elementCount);

		// 使用多线程计算误差
		ThreadPool::parallelFor(0, floatParams.elementCount, [&](int start, int end) -> void {
			for (int elemIdx = start; elemIdx < end; ++elemIdx) {
				float sumAbsErr = 0.0f;
				float sumRelErr = 0.0f;

				// 计算单个元素内所有维度的误差
				for (int dim = 0; dim < floatParams.dimension; ++dim) {
					int idx = elemIdx * floatParams.dimension + dim;

					// 确保索引在有效范围内
					if (idx < source.size() && idx < quantized.size()) {
						// 计算当前维度的绝对误差
						float dimAbsErr = std::abs(source[idx] - quantized[idx]);
						sumAbsErr += dimAbsErr * dimAbsErr;  // 平方和

						// 计算当前维度的相对误差（避免除零）
						if (std::abs(source[idx]) > 1e-10f) {
							float dimRelErr = dimAbsErr / std::abs(source[idx]);
							sumRelErr += dimRelErr * dimRelErr;  // 平方和
						}
					}
				}

				// 计算元素整体的均方根误差
				absError[elemIdx] = std::sqrt(sumAbsErr / floatParams.dimension);
				relError[elemIdx] = std::sqrt(sumRelErr / floatParams.dimension) * 100.0f; // 转换为百分比
			}
			});
	}

	template<typename T>
	static void TotalError(const std::vector<T>& source,
		const std::vector<T>& quantized,
		const FloatParameters& floatParams,
		const FloatErrorControlParameters& errorParams,
		float& keyRelError,
		float& nonKeyRelError)
	{
		// 初始化误差值
		keyRelError = 0.0f;
		nonKeyRelError = 0.0f;

		size_t elementCount = floatParams.elementCount;
		int dimension = floatParams.dimension;

		// 计算数据范围，用于全局相对误差计算
		T maxAbsValue = static_cast<T>(0);
		for (size_t i = 0; i < source.size(); ++i) {
			maxAbsValue = std::max(maxAbsValue, std::abs(source[i]));
		}

		// 避免除零
		maxAbsValue = std::max(maxAbsValue, static_cast<T>(1e-10));

		// 如果不是KeyArea模式，则所有元素一起计算
		if (floatParams.errorMode != ErrorMode::KeyArea) {
			double sumRelError = 0.0;  // 使用double累加提高精度
			size_t validValueCount = 0;

			// 遍历所有值
			for (size_t i = 0; i < source.size() && i < quantized.size(); ++i) {
				// 计算绝对误差
				T error = std::abs(source[i] - quantized[i]);

				// 累加相对误差（基于全局最大值）
				sumRelError += static_cast<double>(error) / static_cast<double>(maxAbsValue);
				validValueCount++;
			}

			// 计算平均全局相对误差（百分比形式）
			keyRelError = (validValueCount > 0) ? static_cast<float>((sumRelError / validValueCount) * 100.0) : 0.0f;
			// 非KeyArea模式下，nonKeyRelError保持为0
			nonKeyRelError = 0.0f;
		}
		// KeyArea模式：分别计算关键区域和非关键区域的误差
		else {
			// 统计关键区域和非关键区域的元素数
			size_t keyElementCount = 0;
			size_t nonKeyElementCount = 0;

			// 关键区域和非关键区域的误差累加
			double keySumRelError = 0.0;  // 使用double累加提高精度
			double nonKeySumRelError = 0.0;

			// 遍历所有元素
			for (size_t elemIdx = 0; elemIdx < elementCount; ++elemIdx) {
				// 判断当前元素是否为关键元素
				bool isKey = (errorParams.isKeyElement.size() > elemIdx) ? errorParams.isKeyElement[elemIdx] : false;

				double elementRelError = 0.0;

				// 计算当前元素的所有维度的相对误差
				for (int dim = 0; dim < dimension; ++dim) {
					size_t idx = elemIdx * dimension + dim;

					// 确保索引在有效范围内
					if (idx < source.size() && idx < quantized.size()) {
						// 计算绝对误差
						T error = std::abs(source[idx] - quantized[idx]);

						// 累加相对误差（基于全局最大值）
						elementRelError += static_cast<double>(error) / static_cast<double>(maxAbsValue);
					}
				}

				// 计算元素的平均相对误差
				elementRelError /= dimension;

				// 根据元素的类型累加到相应的总误差中
				if (isKey) {
					keySumRelError += elementRelError;
					keyElementCount++;
				}
				else {
					nonKeySumRelError += elementRelError;
					nonKeyElementCount++;
				}
			}

			// 计算平均全局相对误差（百分比形式）
			keyRelError = keyElementCount > 0 ? static_cast<float>((keySumRelError / keyElementCount)) : 0.0f;
			nonKeyRelError = nonKeyElementCount > 0 ? static_cast<float>((nonKeySumRelError / nonKeyElementCount)) : 0.0f;
		}
	}

// region deprecated
	/*
	static void CalError(
		std::vector<unsigned char>& dest,
		const void* source, // float* double*
		const MeshOptFloatParameters& floatParams,
		std::vector<std::string>* errorStaResult,
		ErrorStaMode errorStaMode)
	{
		int valueCount = floatParams.elementCount * floatParams.dimension;
		// 误差计算函数
		auto calMSE = [&](const float* source, const float* encoded, const MeshOptFloatParameters& floatParams) -> float {
			float globalError = 0;
			for (int i = 0; i < valueCount; i++) { globalError += std::pow(source[i] - encoded[i], 2); }
			return globalError / valueCount;
			};

		auto calNormalizedL2 = [&](const float* source, const float* encoded, const MeshOptFloatParameters& floatParams) -> float {
			return std::sqrt(calMSE(source, encoded, floatParams));
			};

		auto calMAPE = [&](const float* source, const float* encoded, const MeshOptFloatParameters& floatParams) -> float {
			float globalError = 0;
			for (int i = 0; i < valueCount; i++) {
				globalError += (source[i] == 0 ? 0 : std::abs(source[i] - encoded[i]) / std::abs(source[i]));
			}
			return globalError / valueCount;
			};

		auto calPSNR = [&](const float* source, const float* encoded, const MeshOptFloatParameters& floatParams) -> float {
			float MAX = source[0];
			float MIN = source[0];
			for (int i = 0; i < valueCount; i++) {
				if (source[i] > MAX) {
					MAX = source[i];
				}
				if (source[i] < MIN) {
					MIN = source[i];
				}
			}
			return 10.0 * std::log10(std::pow(MAX - MIN, 2) / calMSE(source, encoded, floatParams));
			};

		auto calULPError = [=](const float* source, const float* encoded, const MeshOptFloatParameters& floatParams, int exp) -> float {
			float error = 0;
			for (int i = 0; i < valueCount; i++) {
				// ulp 指原始值最后一个有效位的单位数值，例如3.14的ulp是0.01
				// abs error / ulp 即 ulp error
				// ulp error有助于分辨量化/四舍五入等round操作的误差 IEEE要求误差 < 1即可
				error += std::abs(source[i] - encoded[i]) / std::pow(2, exp - 23);
			}
			return error / valueCount;
			};

		// 1ULP误差计算
		auto cal1ULPError = [=](const float* source, const float* encoded, const MeshOptFloatParameters& floatParams) -> float {
			return calULPError(source, encoded, floatParams, 0);
			};

		// 1/2ULP误差计算
		auto cal1s2ULPError = [=](const float* source, const float* encoded, const MeshOptFloatParameters& floatParams) -> float {
			return calULPError(source, encoded, floatParams, -1);
			};

		auto calOriginULPError = [=](const float* source, const float* encoded, const MeshOptFloatParameters& floatParams) -> float {
			float error = 0;
			for (int i = 0; i < valueCount; i++) {
				int exponent = ((std::bit_cast<uint32_t>(source[i]) >> 23) & 0xFF) - 127;
				// ulp 指原始值最后一个有效位的单位数值，例如3.14的ulp是0.01
				// abs error / ulp 即 ulp error
				// ulp error有助于分辨量化/四舍五入等round操作的误差 IEEE要求误差 < 1即可
				error += std::abs(source[i] - encoded[i]) / std::pow(2, exponent - 23);
			}
			return error / valueCount;
			};

		switch (errorStaMode)
		{
		case ErrorStaMode::MAPE:
		{
			float err = CalError(source, dest, floatParams, calMAPE);
			std::cout << "float data name: " << debugFloatName << " MSE: " << err << std::endl;
			errorStaResult->push_back(debugFloatName + " MAPE: " + std::to_string(err * 100.0) + "%");
			break;
		}
		case ErrorStaMode::L2:
		{
			float err = CalError(source, dest, floatParams, calNormalizedL2);
			std::cout << "float data name: " << debugFloatName << " L2: " << err << std::endl;
			errorStaResult->push_back(debugFloatName + " 归一化L2: " + std::to_string(err));
			break;
		}
		//case ErrorStaMode::PSNR:
		//{
		//	float err = CalError(source, dest, floatParams, calPSNR);
		//	std::cout << "float data name: " << debugFloatName << " PSNR: " << err << std::endl;
		//	errorStaResult->push_back(debugFloatName + " PSNR: " + std::to_string(err));
		//	break;
		//}
		case ErrorStaMode::OneULP:
		{
			float err = CalError(source, dest, floatParams, cal1ULPError);
			std::cout << "float data name: " << debugFloatName << " 1ULPE: " << err << std::endl;
			errorStaResult->push_back(debugFloatName + " 平均1ULP误差: " + std::to_string(err));
			break;
		}
		case ErrorStaMode::All:
		{
			float err = CalError(source, dest, floatParams, calMAPE);
			std::cout << "float data name: " << debugFloatName << " MAPE: " << err << std::endl;
			errorStaResult->push_back(debugFloatName + " MAPE: " + std::to_string(err));

			err = CalError(source, dest, floatParams, calOriginULPError);
			std::cout << "float data name: " << debugFloatName << " 1ULPE: " << err << std::endl;
			errorStaResult->push_back(debugFloatName + " 平均1ULP误差: " + std::to_string(err));
			break;
		}
		case ErrorStaMode::None:
		default:
			break;
		}
	}
	*/

	
private:
	/*
	static float CalError(const void* origin,
		const std::vector<unsigned char>& encoded,
		const MeshOptFloatParameters& floatParams,
		const std::function<float(const float*, const float*, const MeshOptFloatParameters&)>& CalErrorFunc = nullptr)
	{
		IGsize valueCount = floatParams.elementCount * floatParams.dimension;

		std::vector<float> originFloat(valueCount);
		std::vector<float> encodedFloat(valueCount);

		if (floatParams.valueSize == sizeof(float)) {
			ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
				for (int i = start; i < end; i++) { originFloat[i] = static_cast<const float*>(origin)[i]; }
				});
		}
		else if (floatParams.valueSize == sizeof(double)) {
			ThreadPool::parallelFor(0, valueCount, [&](int start, int end) -> void {
				for (int i = start; i < end; i++) { originFloat[i] = static_cast<const double*>(origin)[i]; }
				});
		}

		//MeshOptDecoder::FloatDecoder(encodedFloat.data(), encoded, floatParams);


	return CalErrorFunc(originFloat.data(), encodedFloat.data(), floatParams);
	}
	*/

// endregion

};

// region deprecated

// 适用于无法直接逐元素设置误差的第三方编码器进行关键/非关键区域的数据压缩
//class ThirdPartyDualPrecisionCodec {
//public:
//	template <class T>
//	static size_t Encoder(
//		std::vector<unsigned char>& dest,
//		const T* source,
//		const MeshOptFloatParameters& floatParams,
//		const std::vector<bool>& isKeyElement,
//		// dest, source, precision, elementCount, elementDim, cmpSize
//		std::function<void(std::vector<unsigned char>&, const T*, double, size_t, size_t, size_t&)> cmpFunc)
//	{
//		int elementCount = floatParams.elementCount;
//		int elementDim = floatParams.dimension;
//		double keyPrecision = floatParams.keyAreaErrorBound;
//		double nonKeyPrecision = floatParams.nonKeyAreaErrorBound;
//
//		BitMap keyElementMap(elementCount);
//		for (int i = 0; i < elementCount; i++)
//		{
//			keyElementMap.set(i, isKeyElement[i]);
//		}
//
//		size_t keyPreCount = 0, nonKeyPreCount = 0;
//		for (int i = 0; i < elementCount; i++)
//		{
//			if (isKeyElement[i]) keyPreCount++;
//			else nonKeyPreCount++;
//		}
//
//		std::vector<T> keyPrecData(keyPreCount * elementDim);
//		std::vector<T> nonKeyPrecData(nonKeyPreCount * elementDim);
//
//		size_t keyIndex = 0, nonKeyIndex = 0;
//		for (size_t i = 0; i < elementCount; i++) {
//			if (isKeyElement[i]) {
//				for (size_t j = 0; j < elementDim; j++) {
//					keyPrecData[keyIndex * elementDim + j] = source[i * elementDim + j];
//				}
//				keyIndex++;
//			}
//			else {
//				for (size_t j = 0; j < elementDim; j++) {
//					nonKeyPrecData[nonKeyIndex * elementDim + j] = source[i * elementDim + j];
//				}
//				nonKeyIndex++;
//			}
//		}
//
//		size_t keyCmpSize = 0, nonKeyCmpSize = 0;
//		std::vector<unsigned char> keyCmpData;
//		std::vector<unsigned char> nonKeyCmpData;
//
//		// dest, source, precision, elementCount, elementDim, cmpSize
//		if (keyPreCount > 0) {
//			cmpFunc(keyCmpData, keyPrecData.data(), keyPrecision, keyPreCount, elementDim, keyCmpSize);
//		}
//		if (nonKeyPreCount > 0) {
//			cmpFunc(nonKeyCmpData, nonKeyPrecData.data(), nonKeyPrecision, nonKeyPreCount, elementDim, nonKeyCmpSize);
//		}
//
//		size_t metaSize =
//			sizeof(size_t) +
//			sizeof(size_t) +
//			sizeof(size_t) +
//			keyElementMap.byteSize();
//
//		size_t cmpSize =  metaSize + keyCmpSize + nonKeyCmpSize;
//
//		dest.resize(cmpSize);
//
//		// 关键元素压缩后尺寸
//		unsigned char* pos = dest.data();
//
//		*((size_t*)pos) = keyCmpSize;
//		pos += sizeof(size_t);
//
//		// 非关键区域压缩后尺寸
//		*((size_t*)pos) = nonKeyCmpSize;
//		pos += sizeof(size_t);
//
//		// 写入位图信息
//		*((size_t*)pos) = keyElementMap.byteSize();
//		pos += sizeof(size_t);
//
//		memcpy(pos, keyElementMap.data(), keyElementMap.byteSize());
//		pos += keyElementMap.byteSize();
//
//		// 写入压缩数据
//		if (keyCmpSize > 0) {
//			memcpy(pos, keyCmpData.data(), keyCmpSize);
//			pos += keyCmpSize;
//		}
//		if (nonKeyCmpSize > 0) {
//			memcpy(pos, nonKeyCmpData.data(), nonKeyCmpSize);
//		}
//
//		return cmpSize;
//	}
//
//	template <class T>
//	static void Decoder(
//		T* dest,
//		unsigned char* encoded,
//		size_t encodedCount,
//		const MeshOptFloatParameters& floatParams,
//		// dest source elementCount elementDim cmpSize
//		std::function<void(const T*, unsigned char*, size_t, size_t, size_t)> decmpFunc)
//	{
//		size_t elementCount = floatParams.elementCount;
//		size_t elementDim = floatParams.dimension;
//
//		unsigned char* pos = encoded;
//
//		size_t keyCmpSize = *((size_t*)pos);
//		pos += sizeof(size_t);
//
//		size_t nonKeyCmpSize = *((size_t*)pos);
//		pos += sizeof(size_t);
//
//		size_t mapSize = *((size_t*)pos);
//		pos += sizeof(size_t);
//
//		BitMap keyElementMap(elementCount);
//		memcpy(const_cast<unsigned char*>(keyElementMap.data()), pos, mapSize);
//		pos += mapSize;
//
//		size_t keyPrecCount = 0, nonKeyPrecCount = 0;
//		for (size_t i = 0; i < elementCount; i++) {
//			if (keyElementMap.get(i)) keyPrecCount++;
//			else nonKeyPrecCount++;
//		}
//
//		// dest source elementCount elementDim cmpSize
//		std::vector<T> keyData;
//		if (keyPrecCount > 0) {
//			decmpFunc(keyData.data(), pos, keyPrecCount, floatParams.dimension, keyCmpSize);
//			pos += keyCmpSize;
//		}
//
//		std::vector<T> nonKeyData;
//		if (nonKeyPrecCount > 0) {
//			decmpFunc(keyData.data(), pos, nonKeyPrecCount, floatParams.dimension, nonKeyCmpSize);
//			pos += nonKeyCmpSize;
//		}
//
//		dest = new T[elementCount * elementDim];
//		std::memset(dest, 0, elementCount * elementDim * sizeof(T));
//
//		size_t keyIdx = 0, nonKeyIdx = 0;
//		for (size_t i = 0; i < elementCount; i++) {
//			for (size_t j = 0; j < elementDim; j++) {
//				size_t dstIdx = i * elementDim + j;
//
//				if (keyElementMap.get(i)) {
//					if (!keyData.empty() && keyIdx < keyPrecCount) {
//						dest[dstIdx] = keyData[keyIdx * elementDim + j];
//					}
//				}
//				else {
//					if (!nonKeyData.empty() && nonKeyIdx < nonKeyPrecCount) {
//						dest[dstIdx] = nonKeyData[nonKeyIdx * elementDim + j];
//					}
//				}
//			}
//
//			if (keyElementMap.get(i)) keyIdx++;
//			else nonKeyIdx++;
//		}
//	}
//
//private:
//	class BitMap {
//	private:
//		std::vector<unsigned char> bits;
//		size_t size;
//
//	public:
//		BitMap(size_t n) : size(n) {
//			bits.resize((n + 7) / 8, 0);
//		}
//
//		void set(size_t pos, bool value) {
//			if (pos >= size) return;
//			if (value)
//				bits[pos / 8] |= (1 << (pos % 8));
//			else
//				bits[pos / 8] &= ~(1 << (pos % 8));
//		}
//
//		bool get(size_t pos) const {
//			if (pos >= size) return false;
//			return (bits[pos / 8] & (1 << (pos % 8))) != 0;
//		}
//
//		const unsigned char* data() const {
//			return bits.data();
//		}
//
//		size_t byteSize() const {
//			return bits.size();
//		}
//	};
//};

//class ZFPFloatCodec {
//public:
//	template <typename T>
//	static void ZFPCompress(
//		std::vector<unsigned char>& dest, const T* data,
//		double precision,
//		size_t elementCount, size_t elementDim,
//		size_t& cmpSize)
//	{
//		// 创建ZFP字段
//		zfp_type type = std::is_same<T, float>::value ? zfp_type_float : zfp_type_double;
//		zfp_field* field = zfp_field_2d(const_cast<T*>(data), type, elementDim, elementCount);
//
//		// 创建ZFP压缩器
//		zfp_stream* zfp = zfp_stream_open(NULL);
//
//		// 设置精度
//		zfp_stream_set_reversible(zfp);
//		//zfp_stream_set_accuracy(zfp, precision);
//
//		// 计算缓冲区大小并分配内存
//		size_t bufsize = zfp_stream_maximum_size(zfp, field);
//		dest.resize(bufsize);
//
//		// 关联比特流与缓冲区
//		bitstream* stream = stream_open(dest.data(), bufsize);
//		zfp_stream_set_bit_stream(zfp, stream);
//
//		// 执行压缩
//		cmpSize = zfp_compress(zfp, field);
//
//		// 清理资源
//		zfp_field_free(field);
//		zfp_stream_close(zfp);
//		stream_close(stream);
//
//		// 调整缓冲区大小为实际压缩大小
//		//dest.resize(cmpSize);
//	}
//
//	// ZFP解压辅助函数，适用于浮点类型
//	template <typename T>
//	static void ZFPDecompress(T* dest, const unsigned char* source, 
//		size_t elementCount, size_t elementDim, size_t cmpSize)
//	{
//		// 创建ZFP字段
//		zfp_type type = std::is_same<T, float>::value ? zfp_type_float : zfp_type_double;
//		zfp_field* field = zfp_field_2d(dest, type, elementDim, elementCount);
//
//		// 创建ZFP解压器
//		zfp_stream* zfp = zfp_stream_open(NULL);
//
//		// 设置比特流
//		bitstream* stream = stream_open(const_cast<unsigned char*>(source), cmpSize);
//		zfp_stream_set_bit_stream(zfp, stream);
//
//		// 执行解压
//		if (!zfp_decompress(zfp, field)) {
//			throw std::runtime_error("ZFP decompression failed");
//		}
//
//		// 清理资源
//		zfp_field_free(field);
//		zfp_stream_close(zfp);
//		stream_close(stream);
//	}
//};

// endregion

IGAME_NAMESPACE_END
#endif