#include "AudioRecorder.h"

AudioRecorder::AudioRecorder()
{
	SampleRate = 44100;
	ExchangingBufferFrames = 512;
	AudioPartDuration = 1;  // 1 Секунда

	// Объект ошибок PortAudio
	// Если данный объект не инициализирован - PortAudio не будет работать
	Err = Pa_Initialize();
	if (Err != paNoError)
	{
		printf("PortAudio error: %s\n", Pa_GetErrorText(Err));
		Pa_Terminate();

		if (Err != paNoError)
		{
			printf("An error occurred while using the portaudio stream\n");
			printf("Error number: %d\n", Err);
			printf("Error message: %s\n", Pa_GetErrorText(Err));
			Err = 1; /* Always return 0 or 1, but no other return codes. */
		}
		IsErrInit = false;
	}
	else
	{
		IsErrInit = true;
	}

	// Сбрасываем флаги
	IsRecoringToFile = false;
	IsRecordingNow = false;
	IsPlayingNow = false;
	ResetDevice(InputDevice);
	ResetDevice(OutputDevice);

	// Настраиваем wav заголовки
	WavFileHeaders.FormatChunkDataSize = 16;
	WavFileHeaders.CompressionCodeType = Compression_Code_PCM;
	WavFileHeaders.NumberOfChannals = this->NumberOfChannels;
	WavFileHeaders.SampleRate = SampleRate;
	WavFileHeaders.SignificantBitsPerSample = 16;
	// Среднее количество байт в секунду
	// делим на 8, что бы из битов получить байты
	WavFileHeaders.AverageBytesPerSecond =
		(SampleRate * WavFileHeaders.SignificantBitsPerSample *
			WavFileHeaders.NumberOfChannals) /
		8;
	WavFileHeaders.BlockAlign =
		WavFileHeaders.SignificantBitsPerSample / 8 * WavFileHeaders.NumberOfChannals;
}

AudioRecorder::~AudioRecorder()
{
	if (InputDevice.AudioData.RecordedSamples != nullptr)
	{
		delete[] InputDevice.AudioData.RecordedSamples;
	}
	if (OutputDevice.AudioData.RecordedSamples != nullptr)
	{
		delete[] OutputDevice.AudioData.RecordedSamples;
	}
	if (IsErrInit)
	{
		FreePAResources(Err);
	}
}

void AudioRecorder::StartRecording()
{
	if (!IsPlayingNow)
	{
		IsRecordingNow = true;
	}
}

void AudioRecorder::StartRecording(const char* FileName)
{
	RecordedFileName = FileName;
	IsRecoringToFile = true;
	StartRecording();
}

bool AudioRecorder::IsRecordInProgress()
{
	return IsRecordingNow;
}

int* AudioRecorder::RecordOnePart(int& SizeOfRecoredArray)
{
	SizeOfRecoredArray = -1;  // Код ошибки
	if (!IsErrInit)
	{
		return nullptr;
	}
	if (!InputDevice.IsDeviceСonfigured)
	{
		if (!SetupDeviceAsInput(InputDevice))
		{
			return nullptr;
		}
	}
	if (InputDevice.IsActiveNow)
	{
		return nullptr;
	}
	InputDevice.IsActiveNow = true;
	// Обнуляем аудиобуффер
	ClearAudioBuffer(InputDevice.AudioData);

	Err = Pa_OpenStream(&InputDevice.Stream,            //
		&InputDevice.DeviceParameters,  // Устройство ввода
		NULL,        // Нет устройства вывода
		SampleRate,  //
		ExchangingBufferFrames,  // Количество Frame переносимых за один
								 // вызов Callback
		paClipOff,  // Не обрезать частоту выше SampleRate(у нас ее нет)
		recordCallback,  // Метод работающий в потоке
		&InputDevice.AudioData);  // Хранилище для записанных данных

	if (!RunPortAudioStream(InputDevice))
	{
		return nullptr;
	}

	SizeOfRecoredArray = InputDevice.AudioData.Size;

	if (IsRecoringToFile)
	{
		AddFileRecordingSamples(InputDevice.AudioData.RecordedSamples,
			SizeOfRecoredArray);
	}

	InputDevice.IsActiveNow = false;

	return InputDevice.AudioData.RecordedSamples;
}

void AudioRecorder::StopRecording()
{

	IsRecordingNow = false;
	while (InputDevice.IsActiveNow)
	{
	}
	if (InputDevice.IsDeviceСonfigured) {
		FreePAResources(InputDevice);
	}
	SaveRecordingSamplesToFile();
}

void AudioRecorder::StartPlaying()
{
	if (!IsRecordingNow)
	{
		IsPlayingNow = true;
	}
	else
	{
		IsRecoringToFile = false;
	}
}

void AudioRecorder::StartPlayingAndRecordingToFile(const char* FileName)
{
	RecordedFileName = FileName;
	IsRecoringToFile = true;
	StartPlaying();
}

bool AudioRecorder::IsPlayingInProgress()
{
	return IsPlayingNow;
}

void AudioRecorder::PlayOnePart(int* PlayingArray, int Size)
{
	if (!IsErrInit)
	{
		return;
	}
	if (!OutputDevice.IsDeviceСonfigured)
	{
		if (!SetupDeviceAsOutput(OutputDevice))
		{
			OutputDevice.IsActiveNow = false;
			return;
		}
	}
	if (OutputDevice.IsActiveNow || Size != OutputDevice.AudioData.Size)
	{
		return;
	}
	// Копируем массив из входного параметра
	std::move(PlayingArray, PlayingArray + Size, OutputDevice.AudioData.RecordedSamples);
	if (IsRecoringToFile)
	{
		AddFileRecordingSamples(OutputDevice.AudioData.RecordedSamples, Size);
	}

	OutputDevice.IsActiveNow = true;

	// Открываем поток записи в аудио устройство
	Err = Pa_OpenStream(&OutputDevice.Stream,  //
		NULL,                  // Нет устройства ввода
		&OutputDevice.DeviceParameters,  // Устройство вывода
		SampleRate,                      //
		ExchangingBufferFrames,  // Количество Frame переносимых за один
								 // вызов Callback-а
		paClipOff,  // Не обрезать частоту выше SampleRate(у нас ее нет)
		playCallback,  // Метод работающий в потоке
		&OutputDevice.AudioData);  // Выводимые аудио данные

	RunPortAudioStream(OutputDevice);
	ClearAudioBuffer(OutputDevice.AudioData);

	OutputDevice.IsActiveNow = false;
}

void AudioRecorder::StopPlaying()
{
	IsPlayingNow = false;
	// Ждем окончания работы устройства
	while (OutputDevice.IsActiveNow)
	{
	}
	if (OutputDevice.IsDeviceСonfigured)
	{
		FreePAResources(OutputDevice);
	}
	SaveRecordingSamplesToFile();
}

int AudioRecorder::GetOnePartSize()
{
	return AudioPartDuration * SampleRate;
}

void AudioRecorder::AddFileRecordingSamples(int* AudioArray, int Size)
{
	for (int i = 0; i < Size; i++)
	{
		FileRecordingSamples.push_back(AudioArray[i]);
	}
}

void AudioRecorder::SaveRecordingSamplesToFile()
{

	if (IsRecoringToFile)
	{
		int* RecordedArray = &FileRecordingSamples[0];
		if (RecordedFileName != nullptr)
		{
			CreateAudioFileFromArray(RecordedFileName, RecordedArray,
				FileRecordingSamples.size());
		}
		else
		{
			CreateAudioFileFromArray("LastPlayedAudio.wav", RecordedArray,
				FileRecordingSamples.size());
		}
		IsRecoringToFile = false;
	}
	FileRecordingSamples.clear();
}

void AudioRecorder::PlayAudio(const char* FileName)
{
	if (!IsErrInit)
	{
		return;
	}
	if (!OutputDevice.IsDeviceСonfigured)
	{
		if (!SetupDeviceAsOutput(OutputDevice))
		{
			return;
		}
	}

	// Считываем данные из файла
	int AudioArraySize;
	OutputDevice.AudioData.RecordedSamples = CreateArrayFromAudioFile(FileName, AudioArraySize);
	if (OutputDevice.AudioData.RecordedSamples == nullptr || AudioArraySize < 0)
	{
		printf("File Read Error");
		FreePAResources(OutputDevice);
		return;
	}
	OutputDevice.AudioData.CurrentIndex = 0;
	OutputDevice.AudioData.Size = AudioArraySize;

	// Открываем поток записи в аудио устройство
	Err = Pa_OpenStream(
		&OutputDevice.Stream,            //
		NULL,                            // Нет устройства ввода
		&OutputDevice.DeviceParameters,  // Устройство вывода
		SampleRate,                      //
		ExchangingBufferFrames,  // Количество Frame переносимых за один вызов Callback-а
		paClipOff,  // Не обрезать частоту выше SampleRate(у нас ее нет)
		playCallback,              // Метод работающий в потоке
		&OutputDevice.AudioData);  // Выводимые аудио данные

	RunPortAudioStream(OutputDevice);

	FreePAResources(OutputDevice);
}

bool AudioRecorder::CreateAudioFileFromArray(const char* FileName, int* AudioArray,
	int Size)
{
	if (Size <= 0)
	{
		return false;
	}

	std::ofstream AudioFile;
	AudioFile.open(FileName, std::ios::binary);

	if (!AudioFile.is_open())
	{
		return false;
	}

	AddRIFFChunkWithoutSize(AudioFile);
	AddFormatChunk(AudioFile);
	AddDataChunk(AudioFile, AudioArray, Size);
	SetRIFFChunkSize(AudioFile);

	AudioFile.close();

	return true;
}

int* AudioRecorder::CreateArrayFromAudioFile(std::string FileName, int& ArraySize)
{
	ArraySize = -1;
	// Открываем файл
	AudioFile<double> file;
	if (!file.load(FileName))
	{
		std::cout << "CAN NOT load file" << std::endl;
		return nullptr;
	}

	// Создаем массив
	int size = file.getNumSamplesPerChannel() * file.getNumChannels() * sizeof(int);
	// Выделяем место в памяти под массив
	int* AudioArray = static_cast<int*>(malloc(size));
	if (!AudioArray)
	{
		std::cout << "CAN NOT create AudioArray\n";
		return nullptr;
	}

	// Заполняем массив данными считанными из wav файла
	int counter = 0;
	for (int i = 0; i < file.getNumSamplesPerChannel(); i++)
	{
		for (int channel = 0; channel < file.getNumChannels(); channel++)
		{
			AudioArray[counter] = CorrectSampleToRead(file.samples[channel][i]);
			counter++;
		}
	}
	ArraySize = counter - 1;
	return AudioArray;
}

void AudioRecorder::ClearAudioBuffer(AudioBuffer& AudioData)
{
	if (AudioData.RecordedSamples != nullptr)
	{
		AudioData.CurrentIndex = 0;
		for (int i = 0; i < AudioData.Size; i++)
		{
			AudioData.RecordedSamples[i] = 0;
		}
	}

}

void AudioRecorder::SetSampleRate(double SampleRate)
{
	if (!InputDevice.IsActiveNow && !OutputDevice.IsActiveNow)
	{
		this->SampleRate = SampleRate;
		if (InputDevice.AudioData.RecordedSamples != nullptr)
		{
			delete[] InputDevice.AudioData.RecordedSamples;
		}
		if (OutputDevice.AudioData.RecordedSamples != nullptr)
		{
			delete[] OutputDevice.AudioData.RecordedSamples;
		}
		ResetDevice(OutputDevice);
		ResetDevice(InputDevice);
	}
}

void AudioRecorder::SetExchangingBufferFrames(int ExchangingBufferFrames)
{
	if (!InputDevice.IsActiveNow && !OutputDevice.IsActiveNow)
	{
		this->ExchangingBufferFrames = ExchangingBufferFrames;
		if (InputDevice.AudioData.RecordedSamples != nullptr)
		{
			delete[] InputDevice.AudioData.RecordedSamples;
		}
		if (OutputDevice.AudioData.RecordedSamples != nullptr)
		{
			delete[] OutputDevice.AudioData.RecordedSamples;
		}
		ResetDevice(OutputDevice);
		ResetDevice(InputDevice);
	}
}

void AudioRecorder::SetAudioPartDuration(double AudioPartDuration)
{
	if (!InputDevice.IsActiveNow && !OutputDevice.IsActiveNow)
	{
		this->AudioPartDuration = AudioPartDuration;
		if (InputDevice.AudioData.RecordedSamples != nullptr)
		{
			delete[] InputDevice.AudioData.RecordedSamples;
		}
		if (OutputDevice.AudioData.RecordedSamples != nullptr)
		{
			delete[] OutputDevice.AudioData.RecordedSamples;
		}
		ResetDevice(OutputDevice);
		ResetDevice(InputDevice);
	}
}

void AudioRecorder::WriteIntToFile(std::ofstream& file, int Value, int Size)
{
	file.write(reinterpret_cast<const char*>(&Value), Size);
}

double AudioRecorder::CorrectSampleToWrite(double Sample)
{
	// Для работы с библиотекой AudioFile
	// нужно провести поправку на максимальную амплитуду сигнала
	double MaxAmplitude = pow(2, WavFileHeaders.SignificantBitsPerSample - 1) - 1;
	// Для записи в файл необходимо разделить записываемый Sample
	// на максимальную амплитуду сигнала
	return static_cast<double>(Sample / MaxAmplitude);
}

int AudioRecorder::CorrectSampleToRead(double Sample)
{
	// Для работы с библиотекой AudioFile
	// нужно провести поправку на максимальную амплитуду сигнала
	// 2 * SignificantBitsPerSample подогнано(я хз почему так... скорее всего сломается)
	double MaxAmplitude = pow(2, 2 * WavFileHeaders.SignificantBitsPerSample - 1) - 1;
	// Для чтения из файла необходимо умножить считываемый Sample
	// на максимальную амплитуду сигнала
	return static_cast<int>(Sample * MaxAmplitude);
}

void AudioRecorder::AddRIFFChunkWithoutSize(std::ofstream& file)
{
	// Создаем заголовки Wave файла
	// RESOURCE INTERCHANGE FILE FORMAT
	file << WavFileHeaders.RIFF;
	file << WavFileHeaders.Plug;  // Заглушка для размера блока
	file << WavFileHeaders.WAVE;
}

void AudioRecorder::AddFormatChunk(std::ofstream& file)
{
	// Блок формата
	file << WavFileHeaders.fmt;
	WriteIntToFile(file, WavFileHeaders.FormatChunkDataSize,
		HEADER_SIZE_OF_Chunk_Data_Size);

	WriteIntToFile(file, WavFileHeaders.CompressionCodeType,
		HEADER_SIZE_OF_Compression_Code);
	WriteIntToFile(file, WavFileHeaders.NumberOfChannals,
		HEADER_SIZE_OF_Number_of_channels);
	WriteIntToFile(file, WavFileHeaders.SampleRate, HEADER_SIZE_OF_Sample_rate);
	WriteIntToFile(file, WavFileHeaders.AverageBytesPerSecond,
		HEADER_SIZE_OF_Average_bytes_per_second);
	WriteIntToFile(file, WavFileHeaders.BlockAlign, HEADER_SIZE_OF_Block_align);
	WriteIntToFile(file,                                         //
		WavFileHeaders.SignificantBitsPerSample,      //
		HEADER_SIZE_OF_Significant_bits_per_sample);  //
}

void AudioRecorder::AddDataChunk(std::ofstream& file, int* AudioArray, int Size)
{
	// data блок(блок данных)
	file << WavFileHeaders.data;
	file << WavFileHeaders.Plug;  // Заглушка для размера блока

	// Сохраняем позицию каретки в файле перед записью данных
	// Это нужно для подсчета размера этих данных
	WavFileHeaders.PreDataPosition = file.tellp();

	for (int i = 0; i < Size; i++)
	{
		int CorrectSample = CorrectSampleToWrite(AudioArray[i]);
		WriteIntToFile(file, CorrectSample, 2);
	}

	// Сохраняем позицию каретки в файле после записи данных
	// Это нужно для подсчета размера этих данных
	WavFileHeaders.PostDataPosition = file.tellp();
	// Подсчитываем размер блока данных
	WavFileHeaders.DataChunkDataSize =
		WavFileHeaders.PostDataPosition - WavFileHeaders.PreDataPosition;

	//  Переставляем каретку в файле на начало заголовка size блока data
	file.seekp(WavFileHeaders.PreDataPosition - HEADER_SIZE_OF_Chunk_Data_Size);
	// Записываем размер данных в блоке data
	WriteIntToFile(file, WavFileHeaders.DataChunkDataSize,
		HEADER_SIZE_OF_Chunk_Data_Size);
}

void AudioRecorder::SetRIFFChunkSize(std::ofstream& file)
{
	// Подсчитываем размер блока RIFF
	// он равен размер всего файла,
	// минус размер 2-х header-ов "ID" и "Size"
	WavFileHeaders.FileSize =
		file.end - HEADER_SIZE_OF_Chunk_ID - HEADER_SIZE_OF_Chunk_Data_Size;
	//  Переставляем каретку в файле на начало заголовка size блока RIFF
	file.seekp(std::ios::beg + HEADER_SIZE_OF_Chunk_ID);
	// Записываем размер данных в блоке RIFF
	WriteIntToFile(file, WavFileHeaders.FileSize, HEADER_SIZE_OF_Chunk_Data_Size);
}

bool AudioRecorder::SetupDeviceAsInput(AudioDevice& Device)
{
	Device.IsDeviceСonfigured = false;
	int TotalFrames;
	int NumSamples;
	int NumBytes;

	fflush(stdout);

	// Настраиваем формат записываемого аудио
	TotalFrames = AudioPartDuration * SampleRate;
	Device.AudioData.Size = TotalFrames;
	Device.AudioData.CurrentIndex = 0;
	NumSamples = TotalFrames * NumberOfChannels;
	NumBytes = NumSamples * sizeof(SAMPLE);
	if (Device.AudioData.RecordedSamples == nullptr)
	{
		// Выделить область памяти под буффер размером NumBytes
		Device.AudioData.RecordedSamples = new SAMPLE[NumBytes];
	}
	if (Device.AudioData.RecordedSamples == nullptr)
	{
		FreePAResources(Device);
		return false;
	}
	// Обнуляем аудиобуффер
	ClearAudioBuffer(Device.AudioData);

	// Подключаем устройство ввода
	Device.DeviceParameters.device = Pa_GetDefaultInputDevice();
	if (Device.DeviceParameters.device == paNoDevice)
	{
		// printf("Error: No default input device.\n");
		FreePAResources(Device.AudioData);
		return false;
	}
	// Настраиваем устройство ввода
	Device.DeviceParameters.channelCount = NumberOfChannels;
	Device.DeviceParameters.sampleFormat = PA_SAMPLE_TYPE;
	Device.DeviceParameters.suggestedLatency =
		Pa_GetDeviceInfo(Device.DeviceParameters.device)->defaultLowInputLatency;
	Device.DeviceParameters.hostApiSpecificStreamInfo = NULL;

	Device.IsDeviceСonfigured = true;
	return true;
}

bool AudioRecorder::SetupDeviceAsOutput(AudioDevice& Device)
{
	Device.IsDeviceСonfigured = false;
	int TotalFrames;
	int NumSamples;
	int NumBytes;

	// Подключаем устройство вывода
	Device.DeviceParameters.device = Pa_GetDefaultOutputDevice();
	if (Device.DeviceParameters.device == paNoDevice)
	{
		FreePAResources(Device.AudioData);
		return false;
	}
	// Настраиваем устройство вывода
	Device.DeviceParameters.channelCount = NumberOfChannels;
	Device.DeviceParameters.sampleFormat = PA_SAMPLE_TYPE;
	Device.DeviceParameters.suggestedLatency =
		Pa_GetDeviceInfo(Device.DeviceParameters.device)->defaultLowOutputLatency;
	Device.DeviceParameters.hostApiSpecificStreamInfo = NULL;

	// Настраиваем формат проигрываемого аудио
	TotalFrames = AudioPartDuration * SampleRate;
	Device.AudioData.Size = TotalFrames;
	Device.AudioData.CurrentIndex = 0;
	NumSamples = TotalFrames * NumberOfChannels;
	NumBytes = NumSamples * sizeof(SAMPLE);
	// Выделить область памяти под буффер размером NumBytes, если память еще не выделена
	if (Device.AudioData.RecordedSamples == nullptr)
	{
		Device.AudioData.RecordedSamples = new SAMPLE[NumBytes];
	}
	if (Device.AudioData.RecordedSamples == nullptr)
	{
		FreePAResources(Device);
		return false;
	}
	// Обнуляем аудиобуффер
	ClearAudioBuffer(Device.AudioData);

	Device.IsDeviceСonfigured = true;
	return true;
}

void AudioRecorder::ResetDevice(AudioDevice& Device)
{
	Device.IsActiveNow = false;
	Device.IsDeviceСonfigured = false;
	Device.AudioData.RecordedSamples = nullptr;
	Device.AudioData.CurrentIndex = 0;
	Device.AudioData.Size = 0;
}

void AudioRecorder::FreePAResources(PaError& Err)
{
	Pa_Terminate();

	if (Err != paNoError)
	{
		printf("An error occurred while using the portaudio stream\n");
		printf("Error number: %d\n", Err);
		printf("Error message: %s\n", Pa_GetErrorText(Err));
		Err = 1; /* Always return 0 or 1, but no other return codes. */
		IsErrInit = false;
	}
}

void AudioRecorder::FreePAResources(AudioBuffer& AudioData)
{
	if (AudioData.RecordedSamples != nullptr) /* Существует ли RecordedSamples как объект */
	{
		// Очищаем память(надеюсь)
		memset(AudioData.RecordedSamples, 0,
			AudioData.Size * NumberOfChannels * sizeof(SAMPLE));
	}
}

void AudioRecorder::FreePAResources(AudioDevice& Device)
{
	Device.IsActiveNow = false;
	FreePAResources(Device.AudioData);
	Device.IsDeviceСonfigured = false;
}

bool AudioRecorder::RunPortAudioStream(AudioDevice& Device)
{
	if (Err != paNoError)
	{
		printf("Opern stream error: %s\n", Pa_GetErrorText(Err));

		IsRecordingNow = false;
		IsPlayingNow = false;
		Device.IsDeviceСonfigured = false;
		Device.IsActiveNow = false;

		FreePAResources(Device);
		return false;
	}
	Err = Pa_StartStream(Device.Stream);
	if (Err != paNoError)
	{
		printf("Start stream error: %s\n", Pa_GetErrorText(Err));

		IsRecordingNow = false;
		IsPlayingNow = false;
		Device.IsDeviceСonfigured = false;
		Device.IsActiveNow = false;

		FreePAResources(Device);
		return false;
	}

	fflush(stdout);
	while ((Err = Pa_IsStreamActive(Device.Stream)) == 1)
	{
		// Pa_Sleep(10);
	}
	if (Err < 0)
	{
		printf("Recording/playing error: %s\n", Pa_GetErrorText(Err));

		IsRecordingNow = false;
		IsPlayingNow = false;
		Device.IsDeviceСonfigured = false;
		Device.IsActiveNow = false;

		FreePAResources(Device);
		return false;
	}
	Err = Pa_CloseStream(Device.Stream);
	if (Err != paNoError)
	{
		printf("Closing stream error: %s\n", Pa_GetErrorText(Err));

		IsRecordingNow = false;
		IsPlayingNow = false;
		Device.IsDeviceСonfigured = false;
		Device.IsActiveNow = false;

		FreePAResources(Device);
		return false;
	}
	return true;
}

int AudioRecorder::recordCallback(const void* inputBuffer, void* outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags, void* userData)
{
	// Аудио буффер, в который необходимо записать аудио дорожку полученную от микрофона
	AudioBuffer* AudioData = (AudioBuffer*)userData;

	// Итератор по буферу ввода
	// (через буффер ввода передается аудио дорожка с микрофона)
	const SAMPLE* inputBufferIterator = (const SAMPLE*)inputBuffer;
	// Итератор по аудио буфферу,
	// первым его значением будет текущий элемент в аудио буффере
	SAMPLE* AudioDataIterator =
		&AudioData->RecordedSamples[AudioData->CurrentIndex * NumberOfChannels];

	int IsBufferingFinished;
	// Сколько Frame осталось буфферизировать
	unsigned long FramesLeft = AudioData->Size - AudioData->CurrentIndex;
	// Сколько Frame будет буфферизированно в данном Callback-е
	long BufferedFramesCount;

	// Декларация не используемых элементов
	// (нужна во измежание ошибок)
	(void)outputBuffer;
	(void)timeInfo;
	(void)statusFlags;
	(void)userData;

	// Если осталось буфферизировать меньше Frame-ов, чем размер буфера ввода
	if (FramesLeft < framesPerBuffer)
	{
		// Оставшееся количество Frame, которое надо за буфферизировать = FramesLeft
		BufferedFramesCount = FramesLeft;
		// После выполнения данного Callback-а, завершить Stream
		IsBufferingFinished = paComplete;
	}
	else
	{
		// Если осталось буфферизировать больше Frame-ов, чем размер буфера ввода
		// Оставшееся количество Frame, которое надо за буфферизировать = размер буфера
		// ввода
		BufferedFramesCount = framesPerBuffer;
		// После выполнения данного Callback-а, вызвать его еще раз
		IsBufferingFinished = paContinue;
	}

	long BufferedFrameIndex;
	// Если нет записанного на микрофон звука
	if (inputBuffer == NULL)
	{
		// Записать тишину в аудио буффер
		for (BufferedFrameIndex = 0; BufferedFrameIndex < BufferedFramesCount;
			BufferedFrameIndex++)
		{
			// (Если два канала, то это левое ухо)
			*AudioDataIterator++ = SAMPLE_SILENCE;
			if (NumberOfChannels == 2)
			{
				// (Если два канала, то это правое ухо)
				*AudioDataIterator++ = SAMPLE_SILENCE;
			}
		}
	}
	else
	{
		// Если есть записанный на микрофон звук
		// Записать в аудио буффер этот звук
		for (BufferedFrameIndex = 0; BufferedFrameIndex < BufferedFramesCount;
			BufferedFrameIndex++)
		{
			// (Если два канала, то это левое ухо)
			*AudioDataIterator++ = *inputBufferIterator++;
			if (NumberOfChannels == 2)
			{
				// (Если два канала, то это правое ухо)
				*AudioDataIterator++ = *inputBufferIterator++;
			}
		}
	}
	// Обновляем текущий индекс аудио буфера
	// (увеличиваем его на количество записанных в него Frame-ов)
	AudioData->CurrentIndex += BufferedFramesCount;

	return IsBufferingFinished;
}

int AudioRecorder::playCallback(const void* inputBuffer, void* outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags, void* userData)
{
	// Аудио буффер, который необходимо проиграть
	// (перевести содержимое данного буфера в буфера вывод)
	AudioBuffer* AudioData = (AudioBuffer*)userData;

	// Итератор по аудио буфферу,
	// первым его значением будет текущий элемент в аудио буффере
	SAMPLE* AudioDataIterator =
		&AudioData->RecordedSamples[AudioData->CurrentIndex * NumberOfChannels];
	// Итератор по буфферу вывода
	SAMPLE* OutputDataIterator = (SAMPLE*)outputBuffer;

	// Сколько Frame осталось буфферизировать
	unsigned int FramesLeft = AudioData->Size - AudioData->CurrentIndex;

	// Декларация не используемых элементов
	// (нужна во измежание ошибок)
	(void)inputBuffer;
	(void)timeInfo;
	(void)statusFlags;
	(void)userData;

	unsigned int OutputBufferIndex;
	// Если осталось буфферизировать меньше Frame-ов, чем размер буфера вывода
	if (FramesLeft < framesPerBuffer)
	{
		// Переносим оставшиеся в аудио буффере Frame-ы в буффер вывода
		for (OutputBufferIndex = 0; OutputBufferIndex < FramesLeft; OutputBufferIndex++)
		{
			// (Если два канала, то это левое ухо)
			*OutputDataIterator++ = *AudioDataIterator++;
			if (NumberOfChannels == 2)
			{
				// (Если два канала, то это правое ухо)
				*OutputDataIterator++ = *AudioDataIterator++;
			}
		}
		// Заполняем нулями оставшуюся часть буфера
		for (; OutputBufferIndex < framesPerBuffer; OutputBufferIndex++)
		{
			// (Если два канала, то это левое ухо)
			*OutputDataIterator++ = 0;
			if (NumberOfChannels == 2)
			{
				// (Если два канала, то это правое ухо)
				*OutputDataIterator++ = 0;
			}
		}
		// Обновляем текущий индекс аудио буфера
		// (увеличиваем его на количество считанных из него Frame-ов)
		AudioData->CurrentIndex += FramesLeft;
		// Завершаем буфферизацию
		return paComplete;
	}

	// Если аудио буффер больше буффера вывода
	// то заполняем весь буфер вывода содержимым аудио буффера
	for (OutputBufferIndex = 0; OutputBufferIndex < framesPerBuffer; OutputBufferIndex++)
	{
		// (Если два канала, то это левое ухо)
		*OutputDataIterator++ = *AudioDataIterator++;
		if (NumberOfChannels == 2)
		{
			// (Если два канала, то это правое ухо)
			*OutputDataIterator++ = *AudioDataIterator++;
		}
	}
	// Обновляем текущий индекс аудио буфера
	// (увеличиваем его на количество считанных из него Frame-ов)
	AudioData->CurrentIndex += framesPerBuffer;
	// Продолжаем буфферизацию
	return paContinue;
}
