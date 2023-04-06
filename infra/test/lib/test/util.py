import inspect
import os
import uuid

def mkpath_tree_fatal( path, struct ):
    path = str( path )
    if type( struct ) == type( {} ):
        if not os.path.exists( path ):
            os.makedirs( path )
        for subpath in struct:
            mkpath_tree_fatal( os.path.join( path, subpath ), struct[subpath] )
    elif type( struct ) == type( [] ):
        with open( path, 'w' ) as fp:
            fp.write( '\n'.join( struct ) )
    elif type( struct ) == type( '' ):
        with open( path, 'w' ) as fp:
            fp.write( struct )
    else:
        raise Exception( 'Unknown type struct found ' + type( struct ) )

def testroot():
    return os.path.dirname(
        os.path.dirname(
            os.path.dirname( __file__ )
        )
    )

def __temproot():
    return os.path.join(
        testroot(),
        'TMP'
    )

def testdir():
    caller_frame            = inspect.stack()[1]
    caller_filename_full    = caller_frame.filename
    caller_filename_rel     = os.path.relpath( caller_filename_full, testroot() )

    u = uuid.uuid4()
    return os.path.join( __temproot(), caller_filename_rel + '_' + u.hex )
