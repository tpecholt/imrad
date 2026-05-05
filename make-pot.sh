#!/bin/sh

xgettext src/*.cpp --keyword=Translate:1,1t --keyword=Translate:1c,2,2t --keyword=Translate:1,2,3t --keyword=Translate:1c,2,3,4t -o src/locale/messages.pot
