@title Debug Release ��ʱ�ļ�����
@echo.
@echo         ɾ����ǰĿ¼��������λ�� Debug �� Release Ŀ¼�е��ļ�
@echo                                                         ---wlc 2009/10/12
@echo.
@echo off
@setlocal enabledelayedexpansion
set /a tm=0
set /a size=0 
set /a fa=0
@color 8b
for /r %%i in (*.aps,*.ilk,*.sdf,*.suo,*res,*.unsuccessfulbuild,*.lastbuildstate,*.manifest,*.obj,*.pch,*.pdb,*.tlog,*.log,*.idb) do if exist %%i    @echo Deleteing %%i & del "%%i" && if not exist %%i  (set /a tm+=1 && set /a size+=%%~zi && @title �Ѿ����� !tm!���ļ�...) else (set /a fa+=1)
@echo.
@echo.
@set /a m=%size%/1000000
@set /a la=%size%%%1000000
@title Debug Release ��ʱ�ļ�����
@echo         �������! 
@set /a num = %tm% + %fa%
if %num% EQU 0 (@echo         û����Ҫ������ļ�! ) else (@echo         �ܹ�����ļ� %tm% ��,��ʡ���̿ռ� %m%.%la% �ס�)
if %fa% GTR 0 @echo         %fa% ���ļ����ʧ�ܣ�Ҳ���������ڱ�ʹ��...
@echo.
@echo         %date% %time%
@echo.