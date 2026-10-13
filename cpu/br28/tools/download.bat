

























@echo off
Setlocal enabledelayedexpansion
@echo ********************************************************************************
@echo SDK BR28
@echo ********************************************************************************
@echo %date%


cd /d %~dp0

set OBJDUMP=C:\JL\pi32\bin\llvm-objdump.exe
set OBJCOPY=C:\JL\pi32\bin\llvm-objcopy.exe
set ELFFILE=sdk.elf
set bankfiles=
set LZ4_PACKET=.\lz4_packet.exe

REM %OBJDUMP% -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
%OBJCOPY% -O binary -j .text %ELFFILE% text.bin
%OBJCOPY% -O binary -j .data %ELFFILE% data.bin
%OBJCOPY% -O binary -j .moveable_slot %ELFFILE% mov_slot.bin
%OBJCOPY% -O binary -j .data_code %ELFFILE% data_code.bin
%OBJCOPY% -O binary -j .overlay_aec %ELFFILE% aec.bin
%OBJCOPY% -O binary -j .overlay_aac %ELFFILE% aac.bin
%OBJCOPY% -O binary -j .overlay_aptx %ELFFILE% aptx.bin
%OBJCOPY% -O binary -j .psr_data_code %ELFFILE% psr_data_code.bin

%OBJCOPY% -O binary -j .common %ELFFILE% common.bin


%OBJDUMP% -section-headers -address-mask=0x1ffffff %ELFFILE%
REM %OBJDUMP% -t %ELFFILE% > symbol_tbl.txt

copy /b text.bin + data.bin + mov_slot.bin + data_code.bin + aec.bin + aac.bin + psr_data_code.bin app.bin


del !bankfiles! common.bin text.bin data.bin bank.bin
copy .\effect_file\music_base.bin eq_cfg_hw.bin


call download/adapter/download.bat
