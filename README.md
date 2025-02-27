<h2>GPUNest</h2>
Это концептуальный CPG-аддон к CorelDraw для компоновки объектов произвольной формы. Он написан для иследования возможностей современных видеокарт в задачах компоновки. Алгоритм использует GLSL-шейдеры для решения задачи, перекладывая большую часть работы на видеокарту. В этом есть большой минус - с увеличением площади компонуемых объектов быстро возрастают требования к производительности видеокарты, размер листа также влияет на производительность. Перед использованием я <b>настоятельно рекомендую</b> провести тест на объектах размером 100x100мм при размере листа 400x400мм, затем, постепенно увеличивая масштаб можно определить возможности своей видеокарты. В окне предпросмотра можно изменять масштаб колёсиком мыши и "перетаскивать" камеру левой кнопкой. Выбраные параметры сохраняются в <a href=https://community.coreldraw.com/sdk/api/draw/17.4/p/application.globaluserdata>GlobalUserData</a>.
<h2>Системные требования</h2>
<table  style="font-size:100%"><tr><td>Операционная система:<td>Windows 7 или выше
<tr><td>Программное обеспечение:<td>CorelDraw версии 17 или выше
<tr><td>Видеокарта:<td>Совместимая с OpenGL 4.6
<tr><td>Процессор:<td>С поддержкой SSE4.1</table>
<h2>Установка</h2>
<b>x86:</b>  Скопировать файл <a href=https://github.com/fersatgit/GPUNest/releases/download/v1.0/GPUNest_x86.cpg>GPUNest_x86.cpg</a> в каталог "Draw\Plugins", если такого каталога нет - его необходимо создать<p>
<b>x64:</b>  Скопировать файл <a href=https://github.com/fersatgit/GPUNest/releases/download/v1.0/GPUNest_x64.cpg>GPUNest_x64.cpg</a> в каталог "Programs64\Addons"
<h2>Работа с аддоном</h2><ol>
<li>Выделить объекты для компоновки. Аддон позволяет работать с кривыми, эллипсами, прямоугольниками и растрами, остальные объекты обрабатываются как прямоугольники. Группа объектов считается одним неразрывным объектом.
<li>Запустить аддон при помощи иконки <img src=icon.ico> на панели "Стандарт"
<li>В появившемся диалоговом окне настроить параметры компоновки:<br>
<ul><li>В полях ширина и высота задаётся размер листа в миллиметрах. Галочка "разместить внутри наибольшей фигуры" позволяет использовать наибольший объект в качестве листа для компоновки.
<li>Стратегия задаёт способ выбора оптимального положения объектов. В ограниченых пространствах хорошо работает стратегиия "по карте высот" в остальных случаях лучше подойдёт стратегия "по выссоте". На эффективность компоновки также сильно влияет шаг поворота (количество вращений). Зачастую шаг в 90 градусов может оказаться предпочтительнее (это зависит от формы компонуемых объектов).
<li>Количество вращений задаёт шаг поворота объектов при переборе вариантов расположения. Например при количестве вращений равном 64, шаг поворота будет равен 360/64=5.625 градуса.
<li>Минимально расстояние между объектами задаётся в миллиметрах.</ul>
<li>После нажатия кнопки "Применить" начнётся процесс компоновки.
<li>По завершении компоновки нужно нажать "Ok" для того, чтобы переместить объекты в CorelDraw.</ol>
<p><img src=1.gif>
<h2>Известные проблемы</h2><ul>
<li>Если после запуска диалогового окна настройки панели команд (Инструменты->Параметры->Рабочее пространство->Настройка->Панели команд) выбрать "Отмена" - иконка аддона будет изменена на иконку по-умолчанию. Лечится перезапуском CorelDraw.
<li>Компоновщик может занизить расстояние между объектами. Рекомендуется задавать немного большее расстояние в настройках.
<li>При компоновке объектов большой площади (особенно на слабых видеокартах) операционная система может прервать выполнение шейдера по таймауту. Если это произошло, то для восстановления работоспособности аддона нужно будет перезапустить CorelDraw. Чтобы отключить лимитирование вы можете создать ключ реестра с именем TdrLevel типом'+' REG_DWORD и значением 0 в разделе "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\GraphicsDrivers" и перезагрузить компьютер. Но учтите, что в таком случае выполнение тяжёлой компоновки уже не удастся прервать (операционная система перестанет отвечать на запросы до тех пор, пока компоновка не завершится).
<li>Максимальное количество листов - 64, количество вращений - 64, расстояние между объектами - 50мм.</ul>
