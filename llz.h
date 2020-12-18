
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! are being used by Doxygen to document the
    software.  Dashes in these comment blocks are used to create bullet lists.  The lack of
    blank lines after a block of dash preceeded comments means that the next block of dash
    preceeded comments is a new, indented bullet list.  I've tried to keep the Doxygen
    formatting to a minimum but there are some other items (like <br> and <pre>) that need
    to be left alone.  If you see a comment that starts with / * ! and there is something
    that looks a bit weird it is probably due to some arcane Doxygen syntax.  Be very
    careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



#ifndef __LLZ_H__
#define __LLZ_H__


#ifdef  __cplusplus
extern "C" {
#endif


#include <time.h>
#include "nvutility.h"


  /*! \mainpage LLZ Data Format

       <br><br>\section disclaimer Disclaimer

       This is a work of the US Government. In accordance with 17 USC 105, copyright
       protection is not available for any work of the US Government.

       Neither the United States Government nor any employees of the United States
       Government, makes any warranty, express or implied, without even the implied
       warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes
       any liability or responsibility for the accuracy, completeness, or usefulness
       of any information, apparatus, product, or process disclosed, or represents
       that its use would not infringe privately-owned rights. Reference herein to
       any specific commercial products, process, or service by trade name, trademark,
       manufacturer, or otherwise, does not necessarily constitute or imply its
       endorsement, recommendation, or favoring by the United States Government. The
       views and opinions of authors expressed herein do not necessarily state or
       reflect those of the United States Government, and shall not be used for
       advertising or product endorsement purposes.


       <br><br>\section intro Introduction

       The LLZ data type is a simple, binary, [time]/lat/lon/depth format.  It includes a 16384 byte ASCII
       header.  The data can be little or big endian.  All Z values are stored as depths not elevations.
       That is, depths are positive and elevations are negative (this is the Naval Oceanographic Office 
       not the Naval Topographic Office).  The ASCII header includes the following keys:

       <pre>
       [VERSION] =
       [ENDIAN] =
       [TIME FLAG] = 
       [UNCERTAINTY FLAG] = 
       [DEPTH UNITS] = 
       [CLASSIFICATION] =
       [DISTRIBUTION] =
       [DECLASSIFICATION] =
       [CLASSIFICATION JUSTIFICATION] =
       [DOWNGRADE] =
       [SOURCE] =
       [COMMENTS] =
       [CREATION_DATE] =
       [LAST MODIFIED DATE] =
       [NUMBER OF RECORDS] =
       [END OF HEADER]
       </pre>

       The [ENDIAN] = field will always be either BIG or LITTLE depending on the byte ordering of the
       binary data in the file.  The time portion of the llz is optional.  If the [TIME FLAG] field is
       equal to 0 or there is no [TIME FLAG] field in the header then time will not be included with
       each lat/lon/depth record.  Same for the [UNCERTAINTY FLAG] field.  Following the 16384 byte
       ASCII header will be [NUMBER OF RECORDS] records.  The latitude, longitude, and depth in each
       record are stored as scaled, signed 32 bit integers.  The status is stored as an unsigned, 16 bit
       integer.  If time is included with each record it will be stored as two 32 bit fields that will
       be POSIX time fields.  That is, the first 32 bit field will contain integer seconds from January
       1, 1970 and the second will contain nanoseconds of the second.  If time is not included in each
       record these fields in the LLZ_REC structure will be set to 0.  If [DEPTH UNITS] is not set it
       defaults to METERS.  The other options for depth units are FEET, FATHOMS, CUBITS (roman), and
       WILLETTS.  These are set in the LLZ_HEADER record as 0, 1, 2, 3, and 4 respectively.  Order and
       scaling of the records is as follows:

       <pre>
       (time_t) seconds from January 1, 1970 - optional
       (long) nanoseconds - optional
       NINT (uncertainty * 10000.0L)
       NINT (latitude * 10000000.0L)
       NINT (longitude * 10000000.0L)
       NINT (depth * 10000.0L)
       (uint16_t) status
       </pre>

       The status bits are explained in llz.h.

*/


#define MAX_LLZ_FILES 32
#define LLZ_HEADER_SIZE 16384


#define LLZ_METERS 0
#define LLZ_FEET 1
#define LLZ_FATHOMS 2
#define LLZ_CUBITS 3
#define LLZ_WILLETTS 4


#define LLZ_NEXT_RECORD -1


typedef struct
{
  char                 version[50];
  uint8_t              time_flag;
  uint8_t              uncertainty_flag;
  uint8_t              depth_units;            /*!<  0 - meters, 1 - feet, 2 - fathoms, 3 - cubits, 4 - willetts */
  char                 classification[50];
  char                 distribution[1000];
  char                 declassification[200];
  char                 class_just[200];
  char                 downgrade[100];
  char                 source[100];
  char                 comments[500];
  char                 creation_date[30];
  char                 modified_date[30];
  int32_t              number_of_records;
} LLZ_HEADER;

typedef struct
{
  time_t               tv_sec;
  long                 tv_nsec;
  float                uncertainty;
  NV_F64_POS           xy;
  float                depth;
  uint32_t             status;
} LLZ_REC;


  int32_t create_llz (const char *path, LLZ_HEADER llz_header);
  int32_t open_llz (const char *path, LLZ_HEADER *llz_header);
  void close_llz (int32_t hnd);
  uint8_t read_llz (int32_t hnd, int32_t recnum, LLZ_REC *data);
  uint8_t append_llz (int32_t hnd, LLZ_REC data);
  uint8_t update_llz (int32_t hnd, int32_t recnum, LLZ_REC data);
  int32_t ftell_llz (int32_t hnd);


#define             LLZ_MANUALLY_INVAL      1       /*!<  Point has been manually marked as invalid : 0000 0000 0000 0001 */
#define             LLZ_FILTER_INVAL        2       /*!<  Point has been automatically marked as invalid : 0000 0000 0000 0010 */
#define             LLZ_INVAL               3       /*!<  Mask to check for either type of invalidity : 0000 0000 0000 0011 */


#ifdef  __cplusplus
}
#endif

#endif
