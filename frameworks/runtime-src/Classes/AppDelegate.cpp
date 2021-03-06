/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "AppDelegate.h"
#include "scripting/lua-bindings/manual/CCLuaEngine.h"
#include "cocos2d.h"
#include "scripting/lua-bindings/manual/lua_module_register.h"
#include "scripting/lua-bindings/manual/network/lua_extensions.h"
#include "scripting/lua-bindings/manual/tolua_fix.h"
#include "scripting/lua-bindings/manual/LuaBasicConversions.h"
#include "lua-bindings/lua_pomelo_auto.hpp"

//get All File Name By Directory
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

//hot upgrade downloading and unzip
#include "MCKernel.h"
#include "ClientKernel.h"
#include "DownAsset.h"
#include "UnZipAsset.h"

//app restart
#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
#include "AppEvent.h"
#endif

// #define USE_AUDIO_ENGINE 1

#if USE_AUDIO_ENGINE
#include "audio/include/AudioEngine.h"
using namespace cocos2d::experimental;
#endif

#include "CSVReader.h"
#include "QrNode.h"

USING_NS_CC;
using namespace std;

AppDelegate::AppDelegate()
{
}

AppDelegate::~AppDelegate()
{
#if USE_AUDIO_ENGINE
    AudioEngine::end();
#endif

#if (COCOS2D_DEBUG > 0) && (CC_CODE_IDE_DEBUG_SUPPORT > 0)
    // NOTE:Please don't remove this call if you want to debug with Cocos Code IDE
    RuntimeEngine::getInstance()->end();
#endif

}

// if you want a different context, modify the value of glContextAttrs
// it will affect all platforms
void AppDelegate::initGLContextAttrs()
{
    // set OpenGL context attributes: red,green,blue,alpha,depth,stencil,multisamplesCount
    GLContextAttrs glContextAttrs = {8, 8, 8, 8, 24, 8, 0 };

    GLView::setGLContextAttrs(glContextAttrs);
}

int toLua_AppDelegate_downFileAsync(lua_State* tolua_S)
{
    
    int argc = lua_gettop(tolua_S);
    if (argc == 4)
    {
        
        const char* szUrl = lua_tostring(tolua_S, 1);
        const char* szSaveName = lua_tostring(tolua_S, 2);
        const char* szSavePath = lua_tostring(tolua_S, 3);
        int handler = toluafix_ref_function(tolua_S, 4, 0);
        if (handler != 0)
        {
            CDownAsset::DownFile(szUrl, szSaveName, szSavePath, handler);
            lua_pushboolean(tolua_S, 1);
            return 1;
        }
        else
        {
            CCLOG("toLua_AppDelegate_setHttpDownCallback hadler or listener is null");
        }
    }
    else
    {
        CCLOG("toLua_AppDelegate_setHttpDownCallback arg error now is %d", argc);
    }
    
    return 0;
}

int toLua_AppDelegate_nativeIsDebug(lua_State* tolua_S)
{
#if (COCOS2D_DEBUG > 0) && (CC_CODE_IDE_DEBUG_SUPPORT > 0)
    lua_pushboolean(tolua_S, 1);
#else
    lua_pushboolean(tolua_S, 0);
#endif
    return 1;
}

int toLua_AppDelegate_onUpDateBaseApp(lua_State* tolua_S)
{
    const char* path = lua_tostring(tolua_S, 1);
    if (path != NULL)
    {
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
        WCHAR wszClassName[256] = {};
        MultiByteToWideChar(CP_ACP, 0, path, strlen(path) + 1, wszClassName, sizeof(wszClassName) / sizeof(wszClassName[0]));
        ShellExecute(NULL, L"open", L"explorer.exe", wszCl=assName, NULL, SW_SHOW);
#endif
        lua_pushboolean(tolua_S, 1);
        return 1;
    }
    return 0;
}

int toLua_AppDelegate_restart(lua_State* tolua_S)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_MAC || CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
    AppEvent event(kAppEventName, APP_EVENT_MENU);
    std::stringstream buf;
    buf << "{\"data\":\"" << "REFRESH_MENU" << "\"";
    buf << ",\"name\":" << "\"menuClicked\"" << "}";
    event.setDataString(buf.str());
    event.setUserData((void*)"REFRESH_MENU");
    Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
#endif
    return 0;
}

int toLua_AppDelegate_unZipAsync(lua_State* tolua_S)
{
    int argc = lua_gettop(tolua_S);
    if (argc == 3)
    {
        const char* file = lua_tostring(tolua_S, 1);
        const char* path = lua_tostring(tolua_S, 2);
        int handler = toluafix_ref_function(tolua_S, 3, 0);
        if (handler != 0)
        {
            CUnZipAsset::UnZipFile(file, path, handler);
            lua_pushboolean(tolua_S, 1);
            return 1;
        }
        else{
            CCLOG("toLua_AppDelegate_unZipAsync error handler is null");
        }
    }
    else{
        CCLOG("toLua_AppDelegate_unZipAsync error argc is %d", argc);
    }
    return 0;
}
std::string toupperCase(const char* pString) {
    std::string copy(pString);
    std::transform(copy.begin(), copy.end(), copy.begin(), ::toupper);
    return copy;
}

//获取文件夹下所有文件名
std::vector<std::string> getAllFileNameByDirectory_android(std::string filePath)
{
    std::vector<std::string> path_vec;
    
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    std::string::size_type pos = filePath.find("assets/");
    std::string relativePath = filePath.substr(pos + strlen("assets/"));
    
    AAssetDir *dir = AAssetManager_openDir(FileUtilsAndroid::getAssetManager(), relativePath.c_str());
    if(dir == NULL)
    {
        CCLOG("getAllFileNameByDirectory_android cannot open %s",filePath.c_str());
        return path_vec;
    }
    
    const char *fileName = nullptr;
    while ((fileName = AAssetDir_getNextFileName(dir)) != nullptr)
    {
        path_vec.push_back(fileName);
    }
    
    AAssetDir_close(dir);
#endif
    return path_vec;
}

std::vector<std::string> getAllFileNameByDirectory(std::string filePath)
{
    filePath = FileUtils::getInstance()->fullPathForFilename(filePath);
    
#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    std::string::size_type pos = filePath.find("assets/");
    if (pos != std::string::npos)
    {
        return getAllFileNameByDirectory_android(filePath);
    }
#endif
    
    std::vector<std::string> path_vec;
    DIR *dp;
    dirent *entry;
    struct stat statbuf;
    
    if((dp=opendir(filePath.c_str()))==NULL)
    {
        CCLOG("toLua_AppDelegate_getAllFileNameByDirectory cannot open %s",filePath.c_str());
        return path_vec;
    }
    chdir(filePath.c_str());
    
    while((entry=readdir(dp))!=NULL)
    {
        stat(entry->d_name,&statbuf);
        if(S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode))
        {
            string d_name = StringUtils::format("%s",entry->d_name);
            if (d_name == "."
                || d_name == ".."
                || toupperCase(d_name.c_str()) == ".DS_STORE"
                || toupperCase(d_name.c_str()) == "THUMBS.DB"
                || toupperCase(d_name.c_str()) == "DESKTOP.INI")
            {
                continue;
            }
            CCLOG("%s",filePath.c_str());
            path_vec.push_back(d_name);
        }
    }
    
    closedir(dp);
    
    return path_vec;
}

int toLua_AppDelegate_getAllFileNameByDirectory(lua_State* tolua_S)
{
    int argc = lua_gettop(tolua_S);
    if (argc == 1)
    {
        const char* path = lua_tostring(tolua_S, 1);
        if (path)
        {
            ccvector_std_string_to_luaval(tolua_S, getAllFileNameByDirectory(path));
            return 1;
        }
        else{
            CCLOG("toLua_AppDelegate_getAllFileNameByDirectory error path is null");
        }
    }
    else{
        CCLOG("toLua_AppDelegate_getAllFileNameByDirectory error argc is %d", argc);
    }
    return 0;
}

int toLua_AppDelegate_copyFile(lua_State* tolua_S)
{
    int argc = lua_gettop(tolua_S);
    if (argc != 1)
    {
        lua_pushboolean(tolua_S, 0);
        return 0;
    }

    const char* filename = lua_tostring(tolua_S, 1);
    if (filename == nullptr)
    {
        lua_pushboolean(tolua_S, 0);
        return 0;
    }
    
    cocos2d::FileUtils* fileUtils = FileUtils::getInstance();
    string destPath = fileUtils->getSearchPaths()[0] + filename;
    
    int nPos = destPath.find_last_of('/');
    if(nPos > 0)
    {
        string strFolder = destPath.substr(0, nPos+1);
        if (false == fileUtils->isDirectoryExist(strFolder))
        {
            fileUtils->createDirectory(strFolder.c_str());
        }
    }
    
    Data data = fileUtils->getDataFromFile(fileUtils->fullPathForFilename(filename));
    FILE *fp = fopen(destPath.c_str(), "w+");
    if (fp)
    {
        size_t size = fwrite(data.getBytes(), sizeof(unsigned char), data.getSize(), fp);
        fclose(fp);
        
        if (size > 0)
        {
            lua_pushboolean(tolua_S, 1);
            return 1;
        }
    }
    
    lua_pushboolean(tolua_S, 0);
    return 0;
}

int toLua_AppDelegate_CSVReaderLine(lua_State* tolua_S)
{
    int argc = lua_gettop(tolua_S);
    if (argc == 2)
    {
        const char* destPath = lua_tostring(tolua_S, 1);
        const char* code = lua_tostring(tolua_S, 2);
        
        const MAP_LINE &ContentMap = CSVReader::getInst()->getContentSrcMap(destPath);
        if (ContentMap.size() <= 0)
        {
            CSVReader::getInst()->parseSrc(destPath);
        }

        lua_pushstring(tolua_S, CSVReader::getInst()->getLineSrcMap(destPath, code).c_str());
        return 1;
    }
    
    return 0;
}

int toLua_AppDelegate_CSVSaveLine(lua_State* tolua_S)
{
    int argc = lua_gettop(tolua_S);
    if (argc == 3)
    {
        const char* destPath = lua_tostring(tolua_S, 1);
        const char* key = lua_tostring(tolua_S, 2);
        const char* line = lua_tostring(tolua_S, 3);
        
        //const MAP_CONTENT &ContentMap = CSVReader::getInst()->getContentMap(destPath);
        //if (ContentMap.size() <= 0)
        //{
           // CCLOG("toLua_AppDelegate_CSVSave no file is %d", destPath);
        //}
        //else
        //{
            CSVReader::getInst()->modifyCSVSrcLine(destPath, key, line);
        //}
        
        return 1;
    }
    
    return 0;
}


// if you want to use the package manager to install more packages, 
// don't modify or remove this function
static int register_all_packages()
{
    lua_State* tolua_S = LuaEngine::getInstance()->getLuaStack()->getLuaState();
    luaopen_lua_extensions(tolua_S);

    lua_register(tolua_S, "downFileAsync", toLua_AppDelegate_downFileAsync);
    lua_register(tolua_S, "onUpDateBaseApp", toLua_AppDelegate_onUpDateBaseApp);
    lua_register(tolua_S, "isDebug", toLua_AppDelegate_nativeIsDebug);
    lua_register(tolua_S, "restart", toLua_AppDelegate_restart);
    lua_register(tolua_S, "unZipAsync", toLua_AppDelegate_unZipAsync);
    lua_register(tolua_S, "getAllFileNameByDirectory", toLua_AppDelegate_getAllFileNameByDirectory);
    lua_register(tolua_S, "copyFile", toLua_AppDelegate_copyFile);
    lua_register(tolua_S, "CSVReaderLine", toLua_AppDelegate_CSVReaderLine);
    lua_register(tolua_S, "CSVSaveLine", toLua_AppDelegate_CSVSaveLine);
    
    return 0; //flag for packages manager
}

bool AppDelegate::applicationDidFinishLaunching()
{
    // set default FPS
    Director::getInstance()->setAnimationInterval(1.0 / 60.0f);

    // register lua module
    auto engine = LuaEngine::getInstance();
    ScriptEngineManager::getInstance()->setScriptEngine(engine);
    lua_State* L = engine->getLuaStack()->getLuaState();
    lua_module_register(L);

    register_all_packages();

    LuaStack* stack = engine->getLuaStack();
    stack->setXXTEAKeyAndSign("2dxLua", strlen("2dxLua"), "XXTEA", strlen("XXTEA"));

    //register custom function
    //LuaStack* stack = engine->getLuaStack();
    //register_custom_function(stack->getLuaState());

    register_all_pomelo(L);
    register_all_QrNode();
    
#if CC_64BITS
    FileUtils::getInstance()->addSearchPath("src/64bit");
#endif
    FileUtils::getInstance()->addSearchPath("src");
    FileUtils::getInstance()->addSearchPath("res");
    if (engine->executeScriptFile("main.lua"))
    {
        return false;
    }
    
    IMCKernel *kernel = GetMCKernel();
    if (kernel)
    {
        kernel->SetLogOut((ILog*)CClientKernel::GetInstance());
        CCLOG("KERNEL SUCCEED:%s", kernel->GetVersion());
    }
    else{
        CCLOG("Load MCKernel Faild************************************************");
        return false;
    }
    Director::getInstance()->getScheduler()->schedule(CClientKernel::globalUpdate, this, 0, kRepeatForever, 0, false, "GlobalUpdate");
    
    return true;
}

// This function will be called when the app is inactive. Note, when receiving a phone call it is invoked.
void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();

#if USE_AUDIO_ENGINE
    AudioEngine::pauseAll();
#endif
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->startAnimation();

#if USE_AUDIO_ENGINE
    AudioEngine::resumeAll();
#endif
}
