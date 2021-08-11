#undef Py_DEBUG
#include <Python.h>
#include "Solve.h"
#include "FieldViewer.h"
#include "PythonHooks.h"
#include <stdio.h>

#include "FileHandler.h"
#include "FieldSolver.h"
#include "Program.h"
#include "FieldTools.h"

struct Solution {
	PyObject_HEAD;
	Solve* m_solve = nullptr;
    bool m_hasSolution = false;
};

static void Solution_dealloc( Solution* self );
static PyObject* Solution_new( PyTypeObject* type, PyObject* args, PyObject* kwds );
static int Solution_init( Solution* self, PyObject* args, PyObject* kwds );
static PyObject* Solution_getEffectiveIndex( Solution* self, PyObject* args );
static PyObject* Solution_display( Solution* self );
static PyObject* Solution_overlap( Solution* self, PyObject* args );
static PyObject* Solution_writeField( Solution* self, PyObject* args );
static PyObject* Solver_solve( PyObject* self, PyObject* args );

static PyMethodDef Solution_methods[] = {
    {"getEffectiveIndex", (PyCFunction)Solution_getEffectiveIndex, METH_VARARGS},
    {"writeField", (PyCFunction)Solution_writeField, METH_VARARGS},
    {"display", (PyCFunction)Solution_display, METH_NOARGS},
    {"overlap", (PyCFunction)Solution_overlap, METH_VARARGS},
    {NULL}
};

static PyTypeObject SolutionType = {
    PyVarObject_HEAD_INIT( NULL, 0 )
    "Solver.Solution",             /* tp_name */
    sizeof( Solution ),             /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)Solution_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "FDFD Solution Object",           /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Solution_methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Solution_init,      /* tp_init */
    0,                         /* tp_alloc */
    Solution_new,                 /* tp_new */
};

static PyObject* getObjectFromDict( PyObject* dict, const char* key )
{
    PyObject* pykey = PyUnicode_FromString( key );
    PyObject* item = PyDict_GetItemWithError( dict, pykey );
    Py_DECREF( pykey );
    return item;
}

static bool getDouble( PyObject* dict, const char* key, double& result )
{
    PyObject* item = getObjectFromDict( dict, key );

    if ( ! item )
        return false;

    result = PyFloat_AsDouble( item );
    return true;
}

static bool getBool( PyObject* dict, const char* key, bool& result )
{
    PyObject* item = getObjectFromDict( dict, key );

    if ( ! item )
        return false;
    
    result = PyObject_IsTrue( item );
    return true;
}

static bool getShort( PyObject* dict, const char* key, short& result )
{
    PyObject* item = getObjectFromDict( dict, key );

    if ( ! item )
        return false;

    result = PyLong_AsLong( item );
    return true;
}

static bool getString( PyObject* dict, const char* key, std::string& result )
{
    PyObject* item = getObjectFromDict( dict, key );

    if ( ! item )
        return false;
    
    PyObject* bytes = PyUnicode_AsASCIIString( item );

    result = PyBytes_AsString( bytes );
    Py_DECREF( bytes );
    return true;
}

static bool getSolverConfigFromDict( PyObject* dict, SolverConfig& config )
{
    if ( ! getShort( dict, "n_points", config.m_points ) )
        return false;
    if ( ! getDouble( dict, "size_of_structure", config.m_size ) )
        return false;
    if ( ! getDouble( dict, "max_index_red", config.m_maxIndexRed ) )
        return false;
    if ( ! getDouble( dict, "max_index_green", config.m_maxIndexGreen ) )
        return false;
    if ( ! getDouble( dict, "max_index_blue", config.m_maxIndexBlue ) )
        return false;
    if ( ! getDouble( dict, "max_neff_guess", config.m_neffGuess ) )
        return false;
    if ( ! getDouble( dict, "wavelength", config.m_wavelength ) )
        return false;
    const char* str;
    if ( ! getString( dict, "geometry_filename", config.m_fileName ) )
        return false;
    if ( ! getBool( dict, "timers", config.m_timers ) )
        return false;
    if ( ! getShort( dict, "num_modes", config.m_modes ) )
        return false;

    return true;
}

static void Solution_dealloc( Solution* self )
{
	delete self->m_solve;
	Py_TYPE( self )->tp_free( (PyObject*)self );
}

static PyObject* Solution_new( PyTypeObject* type, PyObject* args, PyObject* kwds )
{
	Solution* self = (Solution*)type->tp_alloc( type, 0 );

	if ( self )
		self->m_solve = new Solve();

	return (PyObject*)self;
}

static int Solution_init( Solution* self, PyObject* args, PyObject* kwds )
{
    return 0;
}

static PyObject* Solution_getEffectiveIndex( Solution* self, PyObject* args )
{
    if ( ! self->m_solve || ! self->m_hasSolution )
        Py_RETURN_NONE;

    int mode;
    if ( ! PyArg_ParseTuple( args, "i", &mode ) )
    {
        PyErr_SetString( PyExc_TypeError, "Integer must be supplied." );
        return NULL;
    }


    if ( ! self->m_solve->field.inBounds( mode ) )
    {
        PyErr_SetString( PyExc_IndexError, "Supplied mode is out of bounds." );
        return NULL;
    }

    double neff = self->m_solve->field.neff(mode);
    return PyFloat_FromDouble( neff );
}

static PyObject* Solution_display( Solution* self )
{
    if ( ! self->m_solve || ! self->m_hasSolution )
        Py_RETURN_NONE;


    FieldViewer viewer( self->m_solve->mainConfig, self->m_solve->solverConfig, self->m_solve->field, self->m_solve->permativityVector );
    bool quit = viewer.mainLoop();
    PyObject* result = PyTuple_New( 2 );
    PyObject* returnValue = PyBool_FromLong( quit );
    PyObject* selectedMode = PyLong_FromLong( viewer.getCurrentMode() );

    PyTuple_SetItem( result, 0, returnValue );
    PyTuple_SetItem( result, 1, selectedMode );
    return result;
}

static PyObject* Solution_overlap( Solution* self, PyObject* args )
{
    Solution* other;
    int mode;
    if ( ! PyArg_ParseTuple( args, "iO!", &mode, &SolutionType, &other ) )
        return NULL;

    if ( ! other->m_hasSolution || ! self->m_hasSolution )
    {
        PyErr_SetString( PyExc_TypeError, "Both solution objects must have a solution." );
        return NULL;
    }

    if ( other->m_solve->mainConfig.m_width != self->m_solve->mainConfig.m_width
        || other->m_solve->mainConfig.m_height != self->m_solve->mainConfig.m_height )
    {
        PyErr_SetString( PyExc_TypeError, "Both solutions must be the same dimensions." );
        return NULL;
    }

    if ( mode < 0 && mode >= self->m_solve->field.Ex.outerSize() )
    {
        PyErr_SetString( PyExc_TypeError, "Mode is not in solution." );
        return NULL;
    }

    double bestOverlap = -1.0;
    int bestMode = -1;

    for ( int i = 0; i < other->m_solve->field.Ex.outerSize(); i++ )
    {
        double curOverlap = overlap( self->m_solve->field, mode, other->m_solve->field, i, other->m_solve->mainConfig );
        if ( curOverlap > bestOverlap )
        {
            bestOverlap = curOverlap;
            bestMode = i;
        }
    }


    PyObject* result = PyTuple_New( 2 );
    PyObject* pyOverlap = PyFloat_FromDouble( bestOverlap );
    PyObject* pyBestMode = PyLong_FromLong( bestMode );

    PyTuple_SetItem( result, 0, pyOverlap );
    PyTuple_SetItem( result, 1, pyBestMode );

    return result;
}

static PyObject* Solution_writeField( Solution* self, PyObject* args )
{
    const char* filename;
    int mode;

    if ( ! PyArg_ParseTuple( args, "is", &mode, &filename ) )
        return NULL;

    if ( ! self->m_solve || ! self->m_hasSolution )
        Py_RETURN_FALSE;

    bool success = outputFields( 
        self->m_solve->field, 
        mode, 
        self->m_solve->mainConfig.m_width, 
        self->m_solve->mainConfig.m_height, 
        filename );

    if ( success )
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject* Solver_solve( PyObject* self, PyObject* args )
{
    PyObject* dict = nullptr;
    if ( ! PyArg_ParseTuple( args, "O!", &PyDict_Type, &dict ) )
        return NULL;

    PyDictObject* d = (PyDictObject*)dict;
    
    if ( ! dict )
    {
        PyErr_SetString( PyExc_TypeError, "No configuration dictionary supplied." );
        return NULL;
    }
    
    SolverConfig config;
    if ( ! getSolverConfigFromDict( dict, config ) )
    {
        return NULL;
    }

    if ( FileHandler::instance()->getGeometryName() != config.m_fileName )
        FileHandler::instance()->loadGeometry( config.m_fileName );

    FieldSolver solver( Program::instance()->getMainConfig(), config, FileHandler::instance()->getGeometry() );
    Field field;
    if ( solver.solve( field ) )
    {

        Solution* solution = (Solution*)PyObject_CallObject( (PyObject*)&SolutionType, NULL );
        
        //Something went wrong!
        if ( ! solution )
            return NULL;

        solution->m_solve->field = field;
        solution->m_solve->mainConfig = Program::instance()->getMainConfig();
        solution->m_solve->solverConfig = config;
        solution->m_solve->permativityVector = solver.getPermativityVector();
        solution->m_hasSolution = true;

        return (PyObject*)solution;
    }
    
    
    Py_RETURN_NONE;
}

static PyMethodDef Solver_methods[] = {
    {"solve", (PyCFunction)Solver_solve, METH_VARARGS, "Execute solve on geometry"},
    {NULL}
};

static struct PyModuleDef Solvermodule = {
    PyModuleDef_HEAD_INIT,
    "Solver",
    "Maxwell Solver",
    -1,
    Solver_methods
};

PyMODINIT_FUNC PyInit_Solver()
{
    if ( PyType_Ready( &SolutionType ) < 0 )
        return NULL;


    PyObject* m = PyModule_Create( &Solvermodule );
    PyModule_AddObject( m, "Solution", (PyObject*)&SolutionType );

    return m;
}

bool runPythonScript( const char* path )
{
    //wchar_t* program = Py_DecodeLocale( path, NULL );
    //Py_SetProgramName( program );

    if ( PyImport_AppendInittab( "Solver", PyInit_Solver ) == -1 )
        return false;

    Py_Initialize();

    FILE* fp = _Py_fopen( path, "r" );

    if ( ! fp )
        return false;
    //Solution* solution = (Solution*)PyObject_CallObject( (PyObject*)&SolutionType, NULL );
    PyRun_SimpleFile( fp, path );

    Py_Finalize();

    return true;
}