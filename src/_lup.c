#include <Python.h>
#include "lup.h"

#define MODULE_NAME "lup"

struct module_state {
   PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GET_PYTHON_STRING(X) PyUnicode_FromString(X)
#define GET_STATE(X)        ((struct module_state *)PyModule_GetState(X))
#else
#define GET_PYTHON_STRING(X) PyUnicode_FromString(X)
#define GET_STATE(X)        (&_state)
static struct module_state _state;
#endif

/*
 *  Python API
 */

static void del_LuaState(PyObject *self) {
   lua_State *L;
   if ((L = (lua_State*)PyCapsule_GetPointer(self, "LuaState"))) {
      lua_close(L);
   }
}

static PyObject *_lup_LuaState(PyObject *self, PyObject *args) {
   lua_State *L = luaL_newstate();
   luaL_openlibs(L);
   init(L);
   return PyCapsule_New(L, "LuaState", del_LuaState);
}

static PyObject *_lup_process_chunk(PyObject *self, PyObject *args) {
   char *input;
   char *output;
   PyObject *py_luaState;
   PyObject *py_output;
   lua_State *L;

   if (!PyArg_ParseTuple(args, "Os", &py_luaState, &input)) {
      return NULL;
   }
   if (!(L = (lua_State*)PyCapsule_GetPointer(py_luaState, "LuaState"))) {
      return NULL;
   }

   output = process_chunk(L, input);
   if (output) {
      py_output = GET_PYTHON_STRING(output);
      free(output);
   }
   else {
      py_output = Py_None;
      Py_INCREF(py_output);
   }

   return py_output;
}

static PyMethodDef lupMethods[] = {
   {"process_chunk", _lup_process_chunk, METH_VARARGS, "Process this Lua string"},
   {"LuaState", _lup_LuaState, METH_VARARGS, "Create a new Lua State"},
   {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef module_def = {
   PyModuleDef_HEAD_INIT,
   MODULE_NAME,
   NULL,
   sizeof(struct module_state),
   lupMethods,
   NULL,
   NULL,
   NULL,
   NULL
};

PyMODINIT_FUNC PyInit_lup(void) {
   return PyModule_Create(&module_def);
}

#else

DL_EXPORT(void) initlup() {
   PyObject *module = Py_InitModule(MODULE_NAME, lupMethods);
   if(module == NULL) {
      return;
   }

   struct module_state *st = GET_STATE(module);
   st->error = PyErr_NewException(MODULE_NAME ".Error", NULL, NULL);
   if(st->error == NULL) {
      Py_DECREF(module);
   }
}

#endif
