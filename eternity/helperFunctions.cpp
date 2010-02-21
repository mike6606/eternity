#include "helperFunctions.h"

FILE *logFile ;

int logOutputsForConsole ;  // like a "variable declaration..."
int logOutputsForFile ;
int logOutputsForDebugStream ;

// Functions that do error checking
// Just print info about errors that
// just happened.
void logStartup()
{
  logFile = fopen( "lastRunLog.txt", "w" ) ;

  logOutputsForConsole = LOG_ERROR | LOG_WARNING | LOG_INFO ;
  logOutputsForFile = LOG_ERROR | LOG_WARNING | LOG_INFO ;
  logOutputsForDebugStream = LOG_ERROR | LOG_WARNING ;

  info( "Startup" ) ;
}

tm* CurrentTime()
{
  static time_t raw ;
  
  // grab the current time
  time( &raw ) ;

  // Now create that timeinfo struct
  static tm* timeinfo ;
  timeinfo = localtime( &raw ) ;

  return timeinfo ;
}

void log( int logLevel, char *fmt, va_list args )
{
  static char msgBuffer[ 512 ] ;
  vsprintf( msgBuffer, fmt, args ) ;
  
  // write time into timeBuff
  static char timeBuf[ 256 ] ;
  strftime( timeBuf, 255, "%X", CurrentTime() ) ;
  
  char *errLevel ;
  switch( logLevel )
  {
  case LOG_ERROR :
    consoleBrightRed() ;
    errLevel = "error" ;
    break;

  case LOG_WARNING :
    consoleBrightYellow() ;
    errLevel = "warning" ;
    break;

  case LOG_INFO :
    consoleLightGray() ;
    errLevel = "info" ;
    break;
  }

  // Put it all together
  static char buf[ 768 ] ;
  sprintf( buf, "[ %s ][ %s ]:  %s\n", errLevel, timeBuf, msgBuffer ) ;

  // If the error's log level qualifies it to be output to the console based on current console flags..
  if( logOutputsForConsole & logLevel )
    printf( buf ) ;
  if( logOutputsForFile & logLevel )
    fprintf( logFile, buf ) ;
  


  // Also put it in the Visual Studio debug window
  // this also appears in DEBUGVIEW by Mark Russinovich, which you can get
  // from http://technet.microsoft.com/en-us/sysinternals/bb896647.aspx
  if( logOutputsForDebugStream & logLevel )
  {
    // Add program name in front as well
    sprintf( buf, "[ eternity ][ %s ][ %s ]:  %s\n", errLevel, timeBuf, msgBuffer ) ;
    OutputDebugStringA( buf ) ;
  }
}

void error( char *fmt, ... )
{
  va_list lp ;
  va_start( lp, fmt ) ;
  
  log( LOG_ERROR, fmt, lp ) ;
}

void warning( char *fmt, ... )
{
  va_list lp ;
  va_start( lp, fmt ) ;
  
  log( LOG_WARNING, fmt, lp ) ;
}

void info( char *fmt, ... )
{
  va_list lp ;
  va_start( lp, fmt ) ;
  
  log( LOG_INFO, fmt, lp ) ;
}

void logShutdown()
{
  fprintf( logFile, "-- end" ) ;
  fclose( logFile ) ;
}

void bail( char *msg )
{
  info( "BAIL. %s", msg ) ;
  logShutdown() ; //properly shut down the log
  system( "START notepad.exe lastRunLog.txt" ) ;
  system( "pause" ) ;
  exit( 1 );
}

void FMOD_ErrorCheck( FMOD_RESULT result )
{
  if( result != FMOD_OK )
  {
    error( "FMOD error! (%d) %s", result, FMOD_ErrorString(result) );
  }
}

float randFloat( float a, float b )
{
  return lerp( a, b, randFloat() ) ;
}

// Random float between 0 and 1
float randFloat()
{
  return (float)rand() / RAND_MAX ;
}

float lerp( float a, float b, float t )
{
  return a + ( b - a )*t ;
}

double lerp( double a, double b, double t )
{
  return a + ( b - a )*t ;
}


void addSinewave( short *data, int durationInSamples,
                  int offset, int frequency,
                  short amplitude, bool distortion ) 
{
  static double NTs = 1.0 / 44100.0 ;  // assumes this sampling rate

  // "distortion" is just adding 1 to the
  // sinewave so that it will be all positive, so it will totally 
  // get crazy with distortion when multiplied by amp,
  // if amp is high enough
  if( distortion )
    for( int i = offset ; i < offset + durationInSamples ; i++ )
      data[ i ] += amplitude*( 1 + sin( 2*PI*frequency*i*NTs ) ) ;
  else
    for( int i = offset ; i < offset + durationInSamples ; i++ )
      data[ i ] += amplitude*( sin( 2*PI*frequency*i*NTs ) ) ;
}

void addSquarewave( short *data, int durationInSamples, int offset,
                    int fundamentalFrequency, short amplitude )
{
  static double NTs = 1.0 / 44100.0 ;  // assumes this sampling rate

  // we'll just do a signum on the fundamental
  for( int i = offset ; i < offset + durationInSamples ; i++ )
  {
    // we're only interested in the sign of this
    int up = ( sin( 2*PI*fundamentalFrequency*i*NTs ) > 0 ) ?
      +1 :  // if the sin() is bigger than 0, then +1
      -1 ;  // if the sin() is <= 0, then -1
    
    data[ i ] += up*amplitude ;
  }
}

void addSlidingSinewave( short *data, int durationInSamples,
                         int offset,
                         int frequency1, int frequency2,
                         short amplitude1, short amplitude2,
                         bool distortion )
{
  static double NTs = 1.0 / 44100.0 ;  // assumes this sampling rate

  if( distortion )
    for( int i = offset ; i < offset + durationInSamples ; i++ )
    {
      double percentageAlong = (double)i/durationInSamples ;
      double freq = lerp( frequency1, frequency2, percentageAlong ) ;
      double amp = lerp( amplitude1, amplitude2, percentageAlong ) ;
      data[ i ] += amp*( 1 + sin( 2*PI*freq*i*NTs ) ) ;
    }
  else
    for( int i = offset ; i < offset + durationInSamples ; i++ )
    {
      double percentageAlong = (double)i/durationInSamples ;
      double freq = lerp( frequency1, frequency2, percentageAlong ) ;
      double amp = lerp( amplitude1, amplitude2, percentageAlong ) ;
      data[ i ] += amp*( sin( 2*PI*freq*i*NTs ) ) ;
    }
}

void addSlidingSquarewave( short *data, int durationInSamples, int offset,
                           int fundamentalFrequency1, int fundamentalFrequency2,
                           short amplitude1, short amplitude2 )
{
  static double NTs = 1.0 / 44100.0 ;  // assumes this sampling rate

  // As we work along our way, the % of freq1 or freq2 is dictated
  double percentageAlong = 0.0 ;

  // we'll just do a signum on the fundamental
  for( int i = offset ; i < offset + durationInSamples ; i++ )
  {
    percentageAlong = (double)i/durationInSamples ;

    // lerp between two freqs
    double freq = lerp( fundamentalFrequency1, fundamentalFrequency2, percentageAlong ) ;
    double amp  = lerp( amplitude1, amplitude2, percentageAlong ) ;

    // we're only interested in the sign of this
    int up = ( sin( 2*PI*freq*i*NTs ) > 0 ) ?
      +1 :  // if the sin() is bigger than 0, then +1
      -1 ;  // if the sin() is <= 0, then -1
    
    data[ i ] += up*amp ;
  }
}


void printWindowsLastError( char *msg )
{
  LPSTR errorString = NULL ;

  int result = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM,
                 0,
                 GetLastError(),
                 0,
                 (LPSTR)&errorString,
                 0,
                 0 );

  error( "%s %s", msg, errorString ) ;
  
  LocalFree( errorString ) ;
}

bool DX_CHECK( HRESULT hr, char * msg )
{
  if( FAILED( hr ) )
  {
    error( "%s. %s:  %s",
            msg, DXGetErrorStringA( hr ), DXGetErrorDescriptionA( hr ) ) ;

    return false ;
  }

  else
    return true ;

}