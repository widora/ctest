/* It's a test for control the GPIO output & input, then use IIC control 
* IMIO Inc
* Author : zyc
* Date    : 2017-04-11
*/
#include <stdio.h>
#include <string.h>
#include "call_python.h"
#include "/home/zyc/Documents/openwrt_widora/staging_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/include/python2.7/Python.h"

void printDict(PyObject* obj) 
{
    if (!PyDict_Check(obj))
        return;

    int i;
    PyObject *k, *keys;
    keys = PyDict_Keys(obj);

    for (i = 0; i < PyList_GET_SIZE(keys); i++) 
    {
        k = PyList_GET_ITEM(keys, i);
        char* c_name = PyString_AsString(k);
        printf("%s\n", c_name);
    }
}

int call_main(char* filename, char* funcname, char* name, char* language) 
{
    Py_Initialize();
    if (!Py_IsInitialized())
        return -1;

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");

    //import model
    PyObject* pModule = NULL;
    pModule = PyImport_ImportModule(filename);
    if (!pModule) 
    {
        printf("Cant open python file!\n");
        return -1;
    }

    //model dic list
    // PyObject* pDict = PyModule_GetDict(pModule);
    // if (!pDict) 
    // {
        // printf("Cant find dictionary.\n");
        // return -1;
    // }
    //printDict(pDict);

    PyObject * pFunc = NULL;
    pFunc = PyObject_GetAttrString(pModule, funcname);  

    PyObject *pArgs = PyTuple_New(2);  
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", name));  
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("s", language));  
    PyObject *pReturn = NULL;    
    pReturn = PyEval_CallObject(pFunc, pArgs);  
    //char* result;    
    //PyArg_Parse(pReturn, "s", &result);  
    //printf("%s\n", result);

    Py_DECREF(pFunc);
    Py_DECREF(pArgs);
    //Py_DECREF(pDict);
    Py_DECREF(pModule);
    Py_Finalize();

    return 0;
}
