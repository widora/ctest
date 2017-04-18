#include <stdio.h>
#include <stdlib.h>
//#include <python2.7/Python.h>
#include <Python.h>

int main(int argc, char *argv[])
{
    //In order to use Python in C file, must to use Py_Initalize to initalize it.
    char *string;
    PyObject *pName, *pModule, *pDict, *pFunc, *pArgs, *pRetVal; 

    Py_Initialize();       
    if (!Py_IsInitialized())
    {
        return -1; 
        printf("Py_Initialize() failed\n");
    }
  
    // load the name by getContext scripts(Note: not use getContext.py)  
    pName = PyString_FromString("getContext");  
    pModule = PyImport_Import(pName);  
    if (!pModule)   
    {  
        printf("can't find getContext.py");  
        getchar();  
        return -1;  
    }  
    pDict = PyModule_GetDict(pModule);  
    if (!pDict)           
    {  
        return -1;  
    }  
  
    // find the function named getSentence()
    pFunc = PyDict_GetItemString(pDict, "getSentence");  
    if (!pFunc || !PyCallable_Check(pFunc))           
    {  
        printf("can't find function [getSentence]");  
        getchar();  
        return -1;  
    }  
  
    // 参数进栈  
    pArgs = PyTuple_New(2);  
    // PyObject* Py_BuildValue(char *format, ...)  
    // s string，  
    // i int  
    // f float  
    // O Python object  b=f(a) 0.1-0.01=0.09 
    PyTuple_SetItem(pArgs, 0, Py_BuildValue("s", "test.wav"));   
    PyTuple_SetItem(pArgs, 1, Py_BuildValue("i",1));   
  
    // 调用Python函数  
    pRetVal = PyObject_CallObject(pFunc, pArgs);  
    printf("function return value : %ld\r\n", PyInt_AsLong(pRetVal));  
  
    Py_DECREF(pName);  
    Py_DECREF(pArgs);  
    Py_DECREF(pModule);  
    Py_DECREF(pRetVal);  
  
    // close Python  
    Py_Finalize();  

    return 0;
}
