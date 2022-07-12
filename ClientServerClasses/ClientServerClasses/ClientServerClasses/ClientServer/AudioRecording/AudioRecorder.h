#pragma once

#include <portaudio.h>
#include <dsound.h>
#include <dsconf.h>
#include <vector>
#include "AudioConst.h"
#include "ExternalLibs/AudioFile.h"

#if _WIN32 || _WIN64
#if _WIN64
#pragma comment(lib, "portaudio_x64.lib")
#else
#pragma comment(lib, "portaudio_x86.lib")
#endif
#endif


class AudioRecorder
{
public:
	/// ## Конструктор по умолчанию
	/// 
	/// В данном конструкторе настраиваются основные параметры для работы со звуком и wav файлами.
	/// А так же инициализируется объект ошибок PortAudio
	AudioRecorder();
	/// ## Деструктор необходим для уничтожения объекта ошибок PortAudio
	~AudioRecorder();

	/// Количество аудио каналов. Должно быть статическим, т.к. используется в статических
	/// Callback-ах.
	static const int NumberOfChannels = 1;

	/// ## Метод начинающий запись с микрофона
	///
	/// Запись с микрофона осущетсвляется следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartRecording();
	/// while(RecordInProgress())
	/// {
	///      int SizeOfRecoredArray=0;
	///      int* BufferArray = GetPartOfRecord(SizeOfRecoredArray);
	///      if(SizeOfRecoredArray>0)
	///      {
	///           std::copy(BufferArray, BufferArray + SizeOfRecoredArray, DataBuffer);
	///           // Работайте с DataBuffer в нем будут находится записанные сэмплы
	///      }
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopRecording();
	/// @endcode
	/// @see IsRecordInProgress() RecordOnePart() StopRecording()
	void StartRecording();
    /// ## Метод начинающий запись с микрофона напрямую в файл
    ///
    /// Запись с микрофона осущетсвляется следующим образом:
    /// @code
    /// int* DataBuffer = new int[GetOnePartSize()];
    ///
    /// // В потоке считывания звука
    /// StartRecording("Recording.wav");
    /// while(RecordInProgress())
    /// {
    ///      int SizeOfRecoredArray=0;
    ///      int* BufferArray = GetPartOfRecord(SizeOfRecoredArray);
    ///      if(SizeOfRecoredArray>0)
    ///      {
    ///           std::copy(BufferArray, BufferArray + SizeOfRecoredArray, DataBuffer);
    ///           // Работайте с DataBuffer в нем будут находится записанные сэмплы
    ///      }
    /// }
    ///
    /// // В параллельном потоке вызовите метод остановки записи
    /// StopRecording();
    /// @endcode
    /// @see IsRecordInProgress() RecordOnePart() StopRecording()
    void StartRecording(const char* FileName);
	/// ## Метод проверяющий идет ли в данный момент запись с микрофона.
	///
	/// Запись с микрофона осущетсвляется следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartRecording();
	/// while(RecordInProgress())
	/// {
	///      int SizeOfRecoredArray=0;
	///      int* BufferArray = GetPartOfRecord(SizeOfRecoredArray);
	///      if(SizeOfRecoredArray>0)
	///      {
	///           std::copy(BufferArray, BufferArray + SizeOfRecoredArray, DataBuffer);
	///           // Работайте с DataBuffer в нем будут находится записанные сэмплы
	///      }
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopRecording();
	/// @endcode
	/// @returns true - если в данный момент идет запись звука, false - если запись звука
	/// не идет
	/// @see StartRecording() RecordOnePart() StopRecording()
	bool IsRecordInProgress();
	/// ## Метод записывающий аудио
	///
	/// Данный метод записывает аудио определенной продолжительности и возвращающий его в
	/// виде массива int.
	///
	/// Продолжительность записываемого отрезка хранится в поле AudioPartDuration и
	/// изменяется методом SetAudioPartDuration()
	/// Данный метод используется при записи с микрофона.
	/// Запись с микрофона осущетсвляется следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartRecording();
	/// while(RecordInProgress())
	/// {
	///      int SizeOfRecoredArray=0;
	///      int* BufferArray = GetPartOfRecord(SizeOfRecoredArray);
	///      if(SizeOfRecoredArray>0)
	///      {
	///           std::copy(BufferArray, BufferArray + SizeOfRecoredArray, DataBuffer);
	///           // Работайте с DataBuffer в нем будут находится записанные сэмплы
	///      }
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopRecording();
	/// @endcode
	/// @param [out] SizeOfRecoredArray Ссылка на размер записанного массива. Если запись
	/// произвести не удалось, размер будет установлен в -1.
	/// @returns Массив int хранящий в себе записанные сэмплы. Если запись произвести не
	/// удалось - метод вернет nullptr.
	/// @see GetOnePartSize() StartRecording() IsRecordInProgress() StopRecording()
	/// @warning Т.к. в случае неудачи данный метод возвращает nullptr,
	/// необходимо проверить SizeOfRecoredArray на то, является ли он положительным
	/// числом.
	int* RecordOnePart(int& SizeOfRecoredArray);
	/// ## Метод заканчивающий запись с микрофона
	///
	/// Запись с микрофона осущетсвляется следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartRecording();
	/// while(RecordInProgress())
	/// {
	///      int SizeOfRecoredArray=0;
	///      int* BufferArray = GetPartOfRecord(SizeOfRecoredArray);
	///      if(SizeOfRecoredArray>0)
	///      {
	///           std::copy(BufferArray, BufferArray + SizeOfRecoredArray, DataBuffer);
	///           // Работайте с DataBuffer в нем будут находится записанные сэмплы
	///      }
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopRecording();
	/// @endcode
	/// @see StartRecording() IsRecordInProgress() RecordOnePart()
	void StopRecording();

	/// ## Метод начинающий воспроизведение звука
	///
	/// Воспроизведение звука производится следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartPlaying();
	/// while(IsPlayingInProgress())
	/// {
	///     PlayOnePart(AudioData, DataBufferSize);
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopPlaying();
	/// @endcode
	/// @see StartPlayingAndRecordingToFile() IsPlayingInProgress() PlayOnePart()
	/// StopPlaying()
	void StartPlaying();
	/// ## Метод начинающий воспроизведение звука с записью воспроизводимого звука в файл
	///
	/// Воспроизведение звука производится следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartPlayingAndRecordingToFile();
	/// while(IsPlayingInProgress())
	/// {
	///     PlayOnePart(AudioData, DataBufferSize);
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopPlaying();
	/// @endcode
	/// @see StartPlaying() IsPlayingInProgress() PlayOnePart() StopPlaying()
	void StartPlayingAndRecordingToFile(const char* FileName);
	/// ## Метод проверяющий идет ли в данный момент воспроизведение аудио.
	///
	/// Воспроизведение звука производится следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartPlaying();
	/// while(IsPlayingInProgress())
	/// {
	///     PlayOnePart(AudioData, DataBufferSize);
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopPlaying();
	/// @endcode
	/// @returns true - если в данный момент идет воспроизведение звука, false - если
	/// воспроизведение звука не идет
	/// @see StartPlaying() StartPlayingAndRecordingToFile() PlayOnePart() StopPlaying()
	bool IsPlayingInProgress();
	/// ## Метод воспроизводящий аудио
	///
	/// Данный метод воспроизводит аудио определенной продолжительности.
	///
	/// Продолжительность воспроизводимого отрезка хранится в поле AudioPartDuration и
	/// изменяется методом SetAudioPartDuration()
	/// Данный метод используется при воспроизведении аудио.
	/// Воспроизведение звука производится следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartPlaying();
	/// while(IsPlayingInProgress())
	/// {
	///     PlayOnePart(AudioData, DataBufferSize);
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopPlaying();
	/// @endcode
	/// @param [in] PlayingArray Воспроизводимый массив сэмплов(int)
	/// @param [in] Size Размер воспроизводимого массива сэмплов
	/// @see GetOnePartSize() StartPlaying() StartPlayingAndRecordingToFile()
	/// IsPlayingInProgress() StopPlaying()
	void PlayOnePart(int* PlayingArray, int Size);
	/// ## Метод заканчивающий воспроизведение звука
	///
	/// Воспроизведение звука производится следующим образом:
	/// @code
	/// int* DataBuffer = new int[GetOnePartSize()];
	///
	/// // В потоке считывания звука
	/// StartPlaying();
	/// while(IsPlayingInProgress())
	/// {
	///     PlayOnePart(AudioData, DataBufferSize);
	/// }
	///
	/// // В параллельном потоке вызовите метод остановки записи
	/// StopPlaying();
	/// @endcode
	/// @see StartPlaying() StartPlayingAndRecordingToFile()
	/// IsPlayingInProgress() PlayOnePart()
	void StopPlaying();

	/// Возвращает количество элементов массива одной проигрываемой/записываемой части
	int GetOnePartSize();

	/// Метод проигрования аудио из файла
	void PlayAudio(const char* FileName);
	/// Создание аудиофайла из int* вручную
	/// @param [in] AudioArray Массив int из которого будет создан аудио файл
	/// @param [in] Size Размер массива AudioArray
	/// @returns Возвращеет true, если удалось создать файл, иначе false
	bool CreateAudioFileFromArray(const char* FileName, int* AudioArray, int Size);
	/// Создание  int* из аудиофайла средствами библиотеки AudioFile
	/// @param [in] FileName Имя файла из которого будут считываться данные.
	/// @param [out] ArraySize Размер созданного массива или -1 в случае неудачного считывания.
	/// @returns Указатель на созданный массив с аудио дорожкои или nullptr в случае не
	/// удачи
	/// @warning Т.к. в случае неудачи данный метод возвращает nullptr,
	/// необходимо проверить полученное значение на nullptr, перед его последующим
	/// применением
	/// @see CreateAudioFileFromArrayRaw()
	int* CreateArrayFromAudioFile(std::string FileName, int& ArraySize);

	/// ## Сеттер частоты дискретизации
	///
	/// @param [in] SampleRate Новая частота дискретизации
	void SetSampleRate(double SampleRate);
	/// ## Сеттер размера буффера обмена между микрофоном и аудио буффером
	///
	/// @param [in] ExchangingBufferFrames Новый размер буффера обмена
	void SetExchangingBufferFrames(int ExchangingBufferFrames);
	/// ## Сеттер продолжительности(в сек.) записываемой/воспроизводимой аудиочасти
	///
	/// При бесконечной записи/воспроизведении, аудио записывается/воспроизводится частями.
	/// Данное поле отвечает, за продолжительность данных частей. 
	/// @param [in] AudioPartDuration Новая продолжительность аудиочасти
	void SetAudioPartDuration(double AudioPartDuration);

private:
	AudioDevice InputDevice;
	AudioDevice OutputDevice;
	double SampleRate;
	/// Размер буффера обмена между аудиодевайсом и программой
	int ExchangingBufferFrames;
	/// При бесконечной записи, аудио записывается частями.
	/// Данное поле отвечает, за продолжительность данных частей
	double AudioPartDuration;

	/// Поля состояния
	bool IsRecordingNow;  ///< Ведется ли сейчас запись
	bool IsPlayingNow;  ///< Ведется ли сейчас воспроизведение аудио
	bool IsRecoringToFile;  ///< Записывается ли воспроизведенное аудио в файл

	/// Вектор сэмплов записываемых в файл
	std::vector<int> FileRecordingSamples;
	/// Имя файла в который будут записаны сэмплы из вектора
	const char* RecordedFileName;

	/// Объект ошибок PortAudio
	/// Если данный объект не инициализирован - PortAudio не будет работать
	PaError Err;
	bool IsErrInit;  ///< Инициализирован ли объект ошибок(может ли PortAudio работать)

	WavHeadars WavFileHeaders;

	/// Метод записи в wav файл
	void WriteIntToFile(std::ofstream& file, int Value, int Size);

	/// Метод корректировки значений записываемых библиотекой AudioFile
	/// Для работы с библиотекой AudioFile нужно провести поправку на максимальную
	/// амплитуду сигнала Для записи в файл необходимо разделить записываемый Sample на
	/// максимальную амплитуду сигнала
	double CorrectSampleToWrite(double Sample);
	/// Метод корректировки значений считываемых библиотекой AudioFile
	/// Для работы с библиотекой AudioFile нужно провести поправку на максимальную
	/// амплитуду сигнала Для чтения из файла необходимо умножить считываемый Sample на
	/// максимальную амплитуду сигнала
	int CorrectSampleToRead(double Sample);

	/// Методы добавляющие заголовки wav в файл
	void AddRIFFChunkWithoutSize(std::ofstream& file);
	void AddFormatChunk(std::ofstream& file);
	void AddDataChunk(std::ofstream& file, int* AudioArray, int Size);
	void SetRIFFChunkSize(std::ofstream& file);

	/// Метод накапливающий проигранные сэмплы
	void AddFileRecordingSamples(int* AudioArray, int Size);
    void SaveRecordingSamplesToFile();

	void ClearAudioBuffer(AudioBuffer& AudioData);

	// При инициализации класса нужно сбросить флаги и указатели Двайсов, 
	// во избежание ошибок обращения к памяти
	void ResetDevice(AudioDevice& Device);

	void FreePAResources(PaError& Err);
	void FreePAResources(AudioBuffer& AudioData);
	void FreePAResources(AudioDevice& Device);

	bool RunPortAudioStream(AudioDevice& Device);

	/// Методы настройки микрофона и колонок/наушников
	bool SetupDeviceAsInput(AudioDevice& Device);
	bool SetupDeviceAsOutput(AudioDevice& Device);

public:
	/// ## Данный Callback вызывается PortAudio через Pa_OpenStream, при записи
	/// звука.
	///
	/// Данному Callback-у через параметр userData передается аудио буффер,
	/// в который нужно записать полученный с микрофона звук.
	///
	/// Сохранение записанного звука происходит не за один раз, а пачками.
	/// Для этого используется буффер ввода.
	/// Суть в чем, данный Callback через буффер ввода получает часть записанной с
	/// микрофона аудио дорожки. Эту дорожку он переносит в аудио буффер. Затем Callback
	/// вызывается снова и через буффер ввода получает, уже следующую часть записанной с
	/// микрофона аудио дорожки. Эту дорожку он так же переносит в аудио буффер. Так
	/// продолжается до полного заполнения аудио буффера. Если дорожка записанная на
	/// микрофон короче чем аудио буффер, то остаток аудио буффера заполнится тишиной.
	/// @param [in] framesPerBuffer Количество frame-ов переносимое из одного буффера в
	/// другой за один вызов Callback-а
	/// @param [in] inputBuffer Буффер ввода, через данный буффер PortAudio передает
	/// дорожку записанную с микрофона
	/// @param [out] userData Аудио буффер, в который нужно записать звук с микрофона
	/// @param outputBuffer Не используется
	/// @param timeInfo Не используется
	/// @param statusFlags Не используется
	/// @returns paContinue(0) если аудио буффер заполнен не до конца, и необходимо
	/// вызвать данный Callback еще раз. paComplete(1) если аудио буффер был полностью
	/// заполнен и можно завершать поток считывания аудио с микрофона
	static int recordCallback(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags, void* userData);
	/// ## Данный Callback вызывается PortAudio через Pa_OpenStream, при воспроизведении
	/// звука.
	///
	/// Данному Callback-у через параметр userData передается аудио буффер,
	/// который нужно воспроизвести через аудио вывод компьютера.
	///
	/// Вывод производится не весь сразу, а пачками. Для этого используется буффер вывода.
	/// Суть в чем, данный Callback переносит часть аудио буффера в буфер вывода,
	/// воспроизводит буффер вывода, очищает буфер вывода, и записывает в него следующую
	/// часть аудио буффера. Так происходит до тех пор, пока аудио буффер не закончится.
	/// Тогда буффер выовда дозаполнится пустотой и воспроизведется в последний раз. После
	/// чего Stream прекратится.
	/// @param [in] userData Аудио буффер, который нужно воспроизвести
	/// @param [in] framesPerBuffer Количество frame-ов переносимое из одного буффера в
	/// другой за один вызов Callback-а
	/// @param [out] outputBuffer Буффер вывода, воспроизводящийся по итогу выполнения
	/// Callback-а
	/// @param inputBuffer Не используется
	/// @param timeInfo Не используется
	/// @param statusFlags Не используется
	/// @returns paContinue(0) если аудио буффер выведен не до конца, и необходимо вызвать
	/// данный Callback еще раз. paComplete(1) если аудио буффер был полностью выведен и
	/// можно завершать поток вывода аудио
	static int playCallback(const void* inputBuffer, void* outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags, void* userData);
};