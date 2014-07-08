About this package
------------------

The package offers a self-contained, autonomous DCF 77 decoder task to
RTuinOS users. DCF 77 is a low frequency radio signal carrying most
accurate time information. It can be received with little hardware effort
in central Europe. Ready to use receivers are available at different
sources for a few Euros. They can be connected to the Arduino board with a
minimum of interfacing circuitry.

The receiver has a digital pulse of low frequency as output. The time
information is encoded in the varying impulse-pause ratio of the pulse.

The main file found in this package, dcf_taskDcf77.c, implements a decoder
for this information. It is designed as a self-contained RTuinOS task.
Self-contained means that it barely interferes with other RTuinOS tasks
and that the integration of this task into existing RTuinOS applications
is safe and easy.

In this package the DCF decoder is integrated into two runnable RTuinOS
applications. The first one, dcf77, is a pure test and development
environment. It compiles the decoder and prints its state and results to
Arduino's Serial Monitor. The other one re-uses an existing RTuinOS
application, that already has real time elements, to demonstrate how easy
a true integration of the DCF decoder actually is.

Running the samples requires a valid installation of RTuinOS 1.0. Copy the
contents of the archive into the folder
<RTuinOSInstallDir>/code/applications of your RTuinOS installation and
open a shell window. CD to the root directory <RTuinOSInstallDir>, where
GNUmakefile is located. Run make like:

make -s build APP=dcf77
make -s build APP=tc14_dcf

make -s upload APP=dcf77
make -s upload APP=tc14_dcf

The make target "upload" must be used only if the Arduino board is
connected. Maybe you have to specify the USB port on the command line,
please refer to the RTuinOS User Guide.

The code can directly be run on an ATmega 2660 board. Other boards might
need modifications, please refer to the RTuinOS User Guide.

For members of the Arduino forum RTuinOS 1.0 is available at
http://forum.arduino.cc/index.php?topic=184593.0; everybody can find it in
GitHub at https://github.com/PeterVranken/RTuinOS for download and who
tends to contribute to the further development will visit
https://github.com/sudar/RTuinOS.

