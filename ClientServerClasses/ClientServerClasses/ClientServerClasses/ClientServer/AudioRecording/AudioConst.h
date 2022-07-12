#pragma once
#include <portaudio.h>

// Виды кодов сжатия
#define Compression_Code_Unknown 0
#define Compression_Code_PCM 1
#define Compression_Code_Microsoft_ADPCM 2
#define Compression_Code_ITU_G_711_alfa_law 6
#define Compression_Code_ITU_G_711_mu_law 7
#define Compression_Code_IMA_ADPCM 17
#define Compression_Code_ITU_G_723_ADPCM 20
#define Compression_Code_GSM_6_10 49
#define Compression_Code_ITU_G_721_ADPCM 64
#define Compression_Code_MPEG 80
#define Compression_Code_Experimental 65535

// Размеры заголовков wav в байтах
#define HEADER_SIZE_OF_Chunk_ID 4
#define HEADER_SIZE_OF_Chunk_Data_Size 4
#define HEADER_SIZE_OF_WAVEID 4
#define HEADER_SIZE_OF_Compression_Code 2
#define HEADER_SIZE_OF_Number_of_channels 2
#define HEADER_SIZE_OF_Sample_rate 4
#define HEADER_SIZE_OF_Average_bytes_per_second 4
#define HEADER_SIZE_OF_Block_align 2
#define HEADER_SIZE_OF_Significant_bits_per_sample 2
#define HEADER_SIZE_OF_Extra_format_bytes 2

#define PA_SAMPLE_TYPE paInt32
#define SAMPLE_SILENCE 0.0f

typedef int SAMPLE;
/// Буффер хранящий записанные сэмплы
typedef struct
{
	int CurrentIndex;  ///< Индекс текущего сэмпла в буффере
	int Size;          ///< Размер буффера
	SAMPLE* RecordedSamples;  ///< Массив записаных сэмплов
} AudioBuffer;

/// Структура девайса записывающего/воспроизводящего звук
struct AudioDevice
{
	PaStreamParameters DeviceParameters;  ///< Настройки девайса
	PaStream* Stream;  ///< Поток в котором будет испролнятся запись/воспроизведение
	AudioBuffer AudioData;  ///< Записанный звук
	bool IsActiveNow;  ///< Записывает/воспроизводит ли звук девайс в данный момент
	bool IsDeviceСonfigured;  ///< Настроен ли девайс
};

/*
wav файлы состоят из специальных блоков
_____________________________
|     id     |     size     |
|---------------------------|
|           data            |
|___________________________|

		  блок формата
_______________________________
|     fmt     |     size      |
|-----------------------------|
|compression code(код сжатия) |
|       количество каналов    |
|        SampleRate           |
|   Average bytes per second  |
|         Block align         |
| Significant bits per sample |
|     Extra format bytes      |
|_____________________________|

	 data блок(блок данных)
_______________________________
|     data    |     size      |
|-----------------------------|
|        audio samples        |
|_____________________________|

Основной блок, он содержит предыдущие 2 блока
 RESOURCE INTERCHANGE FILE FORMAT
_________________________________
|    "RIFF"    |    "size"      |
|-------------------------------|
|           "WAVE"              |
|_______________________________|
||   "fmt "    |    "size"     ||
||-----------------------------||
||compression code(код сжатия) ||
||       количество каналов    ||
||        SampleRate           ||
||   Average bytes per second  ||
||         Block align         ||
|| Significant bits per sample ||
||     Extra format bytes      ||
||_____________________________||
|                               |
|_______________________________|
||    "data"   |    "size"     ||
||-----------------------------||
||        audio samples        ||
||_____________________________||
|_______________________________|

Тобиш файл выглядит как-то так:
RIFF1245WAVEfmt 0010010001AC4458880210data1234МелодияПошлаааааааааааа
*/

struct WavHeadars
{
	/*
	_________________________________
	|     "RIFF"    |     "size"    |
	*/
	const char* RIFF = "RIFF";
	int FileSize;
	/*
	|-------------------------------|
	|           "WAVE"              |
	*/
	const char* WAVE = "WAVE";
	/*
			  блок формата
	 _______________________________
	|     fmt      |     size       |
	*/
	const char* fmt = "fmt ";
	int FormatChunkDataSize;
	/*
	|------------------------------|
	|compression code(код сжатия)  |
	|       количество каналов     |
	|        SampleRate            |
	|   Average bytes per second   |
	|         Block align          |
	| Significant bits per sample  |
	|     Extra format bytes       |
	*/
	int CompressionCodeType;
	int NumberOfChannals;
	int SampleRate;
	int AverageBytesPerSecond;
	int BlockAlign;
	int SignificantBitsPerSample;
	/*
		 data блок(блок данных)
	_______________________________
	|     data    |     size      |
	*/
	const char* data = "data";
	int DataChunkDataSize;

	/// Позиция каретки в файле до записи данных
	/// Это нужно для подсчета размера этих данных
	int PreDataPosition;
	/// Позиция каретки в файле после записи данных
	/// Это нужно для подсчета размера этих данных
	int PostDataPosition;

	/// Заглушка в заголовках
	/// ставится на место еще не вычесленного размера заголовка
	/// после заполнения заголовка, переписывается на вычесленный размер заголовка
	const char* Plug = "----";
};