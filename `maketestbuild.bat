@echo off

pushd C:\msys64\home\Tu\Cataclysm-DDA

call make CCACHE=1 RELEASE=1 MSYS2=1 DYNAMIC_LINKING=1 SDL=1 TILES=1 SOUND=1 LINTJSON=1 ASTYLE=1 RUNTESTS=0

:END
pause

