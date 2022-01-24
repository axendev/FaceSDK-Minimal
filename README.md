# FaceSDK-Minimal
Minimal example of usage 3DiVi FaceSDK


## Сборка  и запуск тестового приложения

### 1. Установка FaceSDK
Описание процесса установки: https://docs.facesdk.3divi.com/docs/getting_started/

Для установки FaceSDK надо скачать инсталятор. В нем выбрать версию 3.12 (не 3.13!), архитектуру linux_aarch64. Указать директорию, куда будет скачан SDK. 
Инсталятор можно запустить на машине с Win, в этом случае надо будет после завершения инсталяции заархивирвоать директорию с установленным SDK, и этот архив уже перенести на таргет машину.

В результате на таргет машине должна быть директория с SDK, имеющуя такой состав файлов/папок:
```
/bin
/conf
/examples
/include
/lib
/license
/linux_aarch64
/python_api
/share
/swift_api
switcher.sh

```

### 2. Копирование лицензионного файла
Скопируйте полученный файл лицензии в директорию 
```
.../[path to your facesdk dir]/license/
```


### 3. Сборка

Используется система сборки CMake. Перед сборкой замените в файле CMakeLists.txt [строку 47](CMakeLists.txt#L47) 
```
/home/firefly/common/face_sdk/include   # <- Change it to your FaceSDK location
```
на директорию, в которую был распакован архив с FaceSDK. Т.е. строка должна принять вид:

```
.../[path to your facesdk dir]/include/
```


В системе должны быть установлены пакеты Boost и OpenCV. Для установки Boost можно использовать команду
```
sudo apt-get install libboost-all-dev
```


Сборка:
```
mkdir build
cd build
cmake ..
make
```

## Использование  приложения
### Редактирование файла конфигурации
Приложение использует файл конфигурации в формате JSON, в котором задаются параметры, влияющие на работу приложения. Этот файл расположен в данном репозитории в пути `config/config.json`
[config.json](config/config.json)
В файле конфигурации требуется изменить пути до файлов FaceSDK:
```
#3     "dll_path": "/home/firefly/common/face_sdk/lib/libfacerec.so",
#4     "conf_dir_path": "/home/firefly/common/face_sdk/conf/facerec",
#8     "online_licence_dir": "/home/firefly/common/face_sdk/license",
#9     "database_dir": "/home/firefly/common/face_sdk/base",
#10    "license_dir": "/home/firefly/common/face_sdk/license",
```

В этих строках надо заменить путь `/home/firefly/common/face_sdk/` на путь до директории с распакованным facesdk. Т.е. строки должны принять вид:
```
    "dll_path": ".../[path to your facesdk dir]/lib/libfacerec.so",
    "conf_dir_path": ".../[path to your facesdk dir]/conf/facerec",
    "online_licence_dir": ".../[path to your facesdk dir]/license",
    "database_dir": ".../[path to your facesdk dir]/base",
    "license_dir": ".../[path to your facesdk dir]/license",
```

Также требуется указать источник видео. Приложение используется класс OpenCV VideoCapture для захвата видео потока, т.е. как источник видео данных можно использовать RTSP ссылку, локальный видео файл либо устройство V4L2.

Источник видео потока указывается в этой строке:
```
#27   "device": "rtsp://admin:astrohn1@192.168.1.108:554/RVi/1/1",
```

### Запуск приложения
Приложение требует передачи одного параметра, содержащего путь (полный или относительный) до JSON файла конфигурации `config/config.json`
```
> fsdk-minimal ../config/config.json
```

При успешной инициализации вывод приложения будет соответствовать виду:
```
Version 1.12 ...
Using config file at /home/firefly/.vs/FsdkMinimal/0079d77e-1491-4a3e-8ccc-a9f7287211ba/src/config/config.json
Init FaceID Lib ...
FaceRec: Use method config: method10v30_recognizer.xml
FaceRec: Use vworker config: video_worker_fdatracker_uld_fda.xml
FaceRec: FacerecService version: 3.11.01
FaceRec: Init facesdk OK
Init FaceID Lib OK
OpenCamera: Try to open rtsp://admin:astrohn1@192.168.1.108:554/RVi/1/1
OpenCamera: VideoCapture Opened
```

При попадании лица в кадр в терминал выводится строка с информацией о номере кадра, номере трека, координатах левого и правого глаз:
```
Tracking event at frame#143 track_id: 0 left-eye pos: (805.95, 490.5) right-eye pos: (947.216, 473.715)
```


## Использование FaceSDK
Документация на SDK: https://docs.facesdk.3divi.com/docs/

Документация на C++ API: http://download.3divi.com/facesdk/archives/latest/docs/english/namespacepbio.html

Работа с FaceSDK состоит 
- инициализации  SDK
- передачи кадров в SDK
- работе с коллбеками SDK

### Инициализация FaceSDK
Инициализация в тестовом примере выполняется в методе [int init_faceid(json config)](src/fsdk-minimal.cpp#L297) в строке 297 файла fsdk-minimal.cpp

Основным элементом FaceSDK, используемым в данном тестовом приложении, является VideoWorker. Этот элемент работает с входящими видео потоками, производит на них детекцию, трекинг и распознавание лиц. Подробнее о нем можно почитать в документации: https://docs.facesdk.3divi.com/docs/development/dev_video_stream_processing

### Передача кадров в FaceSDK
Кадры передаются в объект VideoWorker в методе [addVideoFrame](src/fsdk-minimal.cpp#L266) в строке 266 файла fsdk-minimal.cpp  

Для передачи кадров используется объект класса `pbio::CVRawImage`, принимающий кадр в представлении OpenCV cv::Mat. 
По умолчанию используется формат кадра BGR24, стандартный для OpenCV. FaceSDK имеет возможность использования других форматов, таких как YUV, I420, NV12 и т.п.

При передаче кадра методе [addVideoFrame](src/fsdk-minimal.cpp#L266) возвращает ID кадра, который в дальнейшем используется в коллебках класса VideoWorker.

### Работа с коллбеками FaceSDK
При инициализации VideoWorker в нем регистрируются коллебки, вызывающиеся при наступлении какого либо события. 
Основные типы коллбеков:
- TrackingCallback (http://download.3divi.com/facesdk/archives/latest/docs/english/classpbio_1_1VideoWorker.html#ab495e8b6d067b3f80407c21c6784a064)
- TemplateCreatedCallback (http://download.3divi.com/facesdk/archives/latest/docs/english/classpbio_1_1VideoWorker.html#ad6e9ae2ac149a24d0b682ef9d466ff8b)
- MatchFoundCallback (http://download.3divi.com/facesdk/archives/latest/docs/english/classpbio_1_1VideoWorker.html#a87576f1b70ad14215679a14e4d630e65)
- TrackingLostCallback (http://download.3divi.com/facesdk/archives/latest/docs/english/classpbio_1_1VideoWorker.html#a4f5a3d56e350b80782e6c3ee4928ac53)

Важно отметить, что поскольку процедура "обработки" (детекции лица, трекинга, поиска по базе) переданного в VideWorker кадра занимает время, коллбеки вызываются спустя некоторое время после передачи кадра. Для коллебка трекинга это время обычно составляет 3-5 кадров.
Во всех коллбеках передается номер кадра, для которого они вызваны, в параметре frame_id. При передаче кадра в VideWorker метод addVideoFrame возвращает номер (ID) кадра. Используя пару этих идентификаторов можно устанавливать соответствие между вызовом коллбека и самим кадром.

#### TrackingCallback 
Вызывается каждый раз, когда происходит передача кадра в VideoWorker. В параметрах передаются данные трекинга, содержащие информацию о всех найденных (детектированных) лицах в кадре.
Этот колбек следует использовать для определения положения и углов поворота лица, координат глаз, оценки качества кадра. 
В тестовом примере для каждого найденного лица определяется:
- Рамка вокруг лица (косвенно определяющая координаты лица в кадре) в [строке 138](src/fsdk-minimal.cpp#L138) 
- Координаты глаз в [строке 140](src/fsdk-minimal.cpp#L140) 
- Углы поворота лица [строке 143](src/fsdk-minimal.cpp#L143) 
- Качество сэмпла [строке 145](src/fsdk-minimal.cpp#L145)
- Флаг проверки условий освещенности в кадре [строке 146](src/fsdk-minimal.cpp#L146)
- Флаг проверки углов поворота лица [строке 147](src/fsdk-minimal.cpp#L147)

Подробнее про эти параметры можно почитать в документации на API (http://download.3divi.com/facesdk/archives/latest/docs/english/structpbio_1_1VideoWorker_1_1TrackingCallbackData.html)

Значения качества сэмпла и флага проверки условий освещенности в кадре можно использовать для оценки кадра на пригодность к распознаванию радужки.
Также в FaceSDK есть возможность определния открыт ли глаз или закрыт, описание: https://docs.facesdk.3divi.com/docs/development/dev_face_estimation/#state-of-the-eyes-openclosed

#### TemplateCreatedCallback 
Вызывается каждый раз, когда из сэмпла лица был получен его шаблон (embedding, вектор признаков лица). 

#### MatchFoundCallback 
Вызывается каждый раз, когда был произведен поиск полученного шаблона лица по базе лиц и было найдено совпадение. 
Используется для функции идентификации лица по базе (1:N).

#### TrackingLostCallback
Вызывается каждый раз, когда был прекращен трекинг лица, т.е. оно пропало из кадра либо по каким то причинам перестало детектироваться.

