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
int main() {
    Py_Initialize();
    if (!Py_IsInitialized())
        return -1;
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");
    //导入模块
    PyObject* pModule = PyImport_ImportModule("testpy");
    if (!pModule) {
        printf("Cant open python file!\n");
        return -1;
    }
    //模块的字典列表
    PyObject* pDict = PyModule_GetDict(pModule);
    if (!pDict) {
        printf("Cant find dictionary.\n");
        return -1;
    }
    //打印出来看一下
    printDict(pDict);
    //演示函数调用
    PyObject* pFunHi = PyDict_GetItemString(pDict, "sayhi");
    PyObject_CallFunction(pFunHi, "lhb");
    Py_DECREF(pFunHi);
    //演示构造一个Python对象，并调用Class的方法
    //获取Second类
    PyObject* pClassSecond = PyDict_GetItemString(pDict, "Second");
    if (!pClassSecond) {
        printf("Cant find second class.\n");
        return -1;
    }
    //获取Person类
    PyObject* pClassPerson = PyDict_GetItemString(pDict, "Person");
    if (!pClassPerson) {
        printf("Cant find person class.\n");
        return -1;
    }
    //构造Second的实例
    PyObject* pInstanceSecond = PyInstance_New(pClassSecond, NULL, NULL);
    if (!pInstanceSecond) {
        printf("Cant create second instance.\n");
        return -1;
    }
    //构造Person的实例
    PyObject* pInstancePerson = PyInstance_New(pClassPerson, NULL, NULL);
    if (!pInstancePerson) {
        printf("Cant find person instance.\n");
        return -1;
    }
    //把person实例传入second的invoke方法
    PyObject_CallMethod(pInstanceSecond, "invoke", "O", pInstancePerson);
    //释放
    Py_DECREF(pInstanceSecond);
    Py_DECREF(pInstancePerson);
    Py_DECREF(pClassSecond);
    Py_DECREF(pClassPerson);
    Py_DECREF(pModule);
    Py_Finalize();
    return 0;
}
