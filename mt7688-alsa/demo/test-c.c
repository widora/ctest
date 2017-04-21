/*
 * test.cpp
 *  Created on: 2010-8-12
 *      Author: lihaibo
 */
#include "/usr/include/python2.7/Python.h"
//#include "/home/zyc/Documents/Python-2.7.3/python-mips/include/python2.7/Python.h"
#include <stdio.h>
#include <string.h>

void printDict(PyObject* obj) {
    if (!PyDict_Check(obj))
        return;
    PyObject *k, *keys;
    keys = PyDict_Keys(obj);
    int i;
    for (i = 0; i < PyList_GET_SIZE(keys); i++) {
        k = PyList_GET_ITEM(keys, i);
        char* c_name = PyString_AsString(k);
        printf("%s\n", c_name);
    }
}

int main() 
{
    Py_Initialize();
    if (!Py_IsInitialized())
        return -1;
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("print 'Enter python: '");  
    PyRun_SimpleString("sys.path.append('./')");
    //导入模块
    PyObject* pModule = NULL;
    pModule = PyImport_ImportModule("getContext");
    if (!pModule) {
        printf("Cant open python file!\n");
        return -1;
    }
    PyObject * pFunc = NULL;
    pFunc = PyObject_GetAttrString(pModule,"getSentence");  
    PyObject *pArgs = PyTuple_New(2);  
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", "test.wav"));  
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", "1"));  
    PyObject *pReturn = NULL;    
    pReturn = PyEval_CallObject(pFunc, pArgs);  
    //char* result;    
    //PyArg_Parse(pReturn, "s", &result);  
    //printf("%s\n", result);
/*
    //模块的字典列表
    PyObject* pDict = PyModule_GetDict(pModule);
    if (!pDict) {
        printf("Cant find dictionary.\n");
        return -1;
    }
    //打印出来看一下
    printDict(pDict);
    //演示函数调用
    

    PyObject* pFunHi = PyDict_GetItemString(pDict, "getSentence");
    PyObject_CallFunction(pFunHi, "test.wav");
    Py_DECREF(pFunHi);
*/
    Py_DECREF(pModule);
    Py_Finalize();

    return 0;
}
