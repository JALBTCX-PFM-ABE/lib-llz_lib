
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



#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "llz.h"
#include "swap_bytes.h"
#include "llz_version.h"


typedef struct
{
  FILE          *fp;
  uint8_t       time_flag;            /*!<  This is duplicated in header due to the need to support 1.0 files.  */
  uint8_t       uncertainty_flag;     /*!<  This is duplicated in header due to the need to support 1.0 files.  */
  uint8_t     depth_units;          /*!<  This is duplicated in header due to the need to support 1.0 files.  */
  uint8_t       swap;
  uint8_t       at_end;
  uint8_t       size_changed;
  uint8_t       modified;
  uint8_t       created;
  uint8_t       write;
  LLZ_HEADER    header;
  uint16_t      major_version;
} INTERNAL_LLZ_HEADER;

typedef struct
{
  int32_t      tv_sec;
  int32_t      tv_nsec;
  int32_t      uncertainty;
  int32_t      lat;
  int32_t      lon;
  int32_t      dep;
  uint16_t    stat;
} INTERNAL_LLZ;

static INTERNAL_LLZ_HEADER llzh[MAX_LLZ_FILES];
static uint8_t first;
static int32_t llz_recnum[MAX_LLZ_FILES];


int32_t big_endian ();



/********************************************************************/
/*!

 - Function:    swap_internal_llz

 - Purpose:     Byte swap an internal llz record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/31/06

 - Arguments:   data           -    The internal format llz record to be swapped

 - Returns:     N/A

********************************************************************/

static void swap_internal_llz (INTERNAL_LLZ *data)
{
  swap_int (&data->tv_sec);
  swap_int (&data->tv_nsec);
  swap_int (&data->uncertainty);
  swap_int (&data->lat);
  swap_int (&data->lon);
  swap_int (&data->dep);
  swap_int ((int32_t *) &data->stat);
}


/********************************************************************/
/*!

 - Function:    write_llz_header

 - Purpose:     Write the llz_header to the llz file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/31/06

 - Arguments:   hnd            =    The llz file handle

 - Returns:     N/A

********************************************************************/

static void write_llz_header (int32_t hnd)
{
  uint8_t zero = 0;
  int32_t i, size ;
  char token[1024], version[1024];

  rewind (llzh[hnd].fp);


  fprintf (llzh[hnd].fp, "[VERSION] = %s\n", LLZ_VERSION);


  /* Added version check before the creation of llz files */
  /* In the past, created llz files were defaulted to version 0 (32bit status) */

  strcpy(version, LLZ_VERSION);
  strcpy(token, strtok(version, "V"));
  strcpy(token, strtok(NULL, version));
  strcpy(token, strtok(token, " "));
  llzh[hnd].major_version = (uint16_t) atoi(token);


  if (llzh[hnd].time_flag)
    {
      fprintf (llzh[hnd].fp, "[TIME FLAG] = 1\n");
    }
  else
    {
      fprintf (llzh[hnd].fp, "[TIME FLAG] = 0\n");
    }

  if (llzh[hnd].uncertainty_flag)
    {
      fprintf (llzh[hnd].fp, "[UNCERTAINTY FLAG] = 1\n");
    }
  else
    {
      fprintf (llzh[hnd].fp, "[UNCERTAINTY FLAG] = 0\n");
    }

  switch (llzh[hnd].depth_units)
    {
    case 0:
    default:
      fprintf (llzh[hnd].fp, "[DEPTH UNITS] = METERS\n");
      break;

    case 1:
      fprintf (llzh[hnd].fp, "[DEPTH UNITS] = FEET\n");
      break;

    case 2:
      fprintf (llzh[hnd].fp, "[DEPTH UNITS] = FATHOMS\n");
      break;

    case 3:
      fprintf (llzh[hnd].fp, "[DEPTH UNITS] = CUBITS\n");
      break;

    case 4:
      fprintf (llzh[hnd].fp, "[DEPTH UNITS] = WILLETTS\n");
      break;
    }


  if (big_endian ())
    {
      fprintf (llzh[hnd].fp, "[ENDIAN] = BIG\n");
    }
  else
    {
      fprintf (llzh[hnd].fp, "[ENDIAN] = LITTLE\n");
    }

  fprintf (llzh[hnd].fp, "[CLASSIFICATION] = %s\n", llzh[hnd].header.classification);
  fprintf (llzh[hnd].fp, "[DISTRIBUTION] = %s\n", llzh[hnd].header.distribution);
  fprintf (llzh[hnd].fp, "[DECLASSIFICATION] = %s\n", llzh[hnd].header.declassification);
  fprintf (llzh[hnd].fp, "[CLASSIFICATION JUSTIFICATION] = %s\n", llzh[hnd].header.class_just);
  fprintf (llzh[hnd].fp, "[DOWNGRADE] = %s\n", llzh[hnd].header.downgrade);
  fprintf (llzh[hnd].fp, "[SOURCE] = %s\n", llzh[hnd].header.source);
  fprintf (llzh[hnd].fp, "[COMMENTS] = %s\n", llzh[hnd].header.comments);
  fprintf (llzh[hnd].fp, "[CREATION DATE] = %s\n", llzh[hnd].header.creation_date);
  fprintf (llzh[hnd].fp, "[LAST MODIFIED DATE] = %s\n", llzh[hnd].header.modified_date);

  fprintf (llzh[hnd].fp, "[NUMBER OF RECORDS] = %d\n", llzh[hnd].header.number_of_records);


  fprintf (llzh[hnd].fp, "[END OF HEADER]\n");


  size = LLZ_HEADER_SIZE - ftell (llzh[hnd].fp);

  for (i = 0 ; i < size ; i++) fwrite (&zero, 1, 1, llzh[hnd].fp);
}



/********************************************************************/
/*!

 - Function:    create_llz

 - Purpose:     Create an llz file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/31/06

 - Arguments:
                - path           =    The llz file path
                - llz_header     =    LLZ_HEADER structure to
                                      be written to the file
                                      Note: leave version, number_of_records,
                                      creation_date, and modified_date
                                      empty.  Don't forget to set time_flag,
                                      uncertainty_flag, and depth_units.

 - Returns:
                - The file handle or -1 on error

********************************************************************/

int32_t create_llz (const char *path, LLZ_HEADER llz_header)
{
  int32_t i, hnd;


  /*  The first time through we want to initialize the llz handle array.  */

  if (first)
    {
      for (i = 0 ; i < MAX_LLZ_FILES ; i++) 
        {
          memset (&llzh[i], 0, sizeof (INTERNAL_LLZ_HEADER));
          llzh[i].fp = NULL;
        }
      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  */

  hnd = MAX_LLZ_FILES;
  for (i = 0 ; i < MAX_LLZ_FILES ; i++)
    {
      if (llzh[i].fp == NULL)
        {
          hnd = i;
          llz_recnum[hnd] = 0;
          break;
        }
    }


  if (hnd == MAX_LLZ_FILES)
    {
      fprintf (stderr, "\n\nToo many open llz files!\nTerminating!\n\n");
      exit (-1);
    }


  /*  Check for programs built with version 1.0 of the library (just in case
      time_flag, uncertainty_flag, or depth units has a weird value).  */

  if (llz_header.time_flag != 1) llz_header.time_flag = 0;
  if (llz_header.uncertainty_flag != 1) llz_header.uncertainty_flag = 0;
  if (llz_header.depth_units > 4) llz_header.depth_units = 0;


  llzh[hnd].time_flag = llz_header.time_flag;
  llzh[hnd].uncertainty_flag = llz_header.uncertainty_flag;
  llzh[hnd].depth_units = llz_header.depth_units;


  /*  Open the file and write the header.  */

  if ((llzh[hnd].fp = fopen64 (path, "wb+")) != NULL)
    {
      llzh[hnd].header = llz_header;

      write_llz_header (hnd);
    }
  else
    {
      hnd = -1;
    }


  llzh[hnd].at_end = 1;
  llzh[hnd].size_changed = 1;
  llzh[hnd].modified = 1;
  llzh[hnd].created = 1;
  llzh[hnd].write = 1;
  llzh[hnd].header.number_of_records = 0;

  return (hnd);
}


/********************************************************************/
/*!

 - Function:    open_llz

 - Purpose:     Open an llz file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/31/06

 - Arguments:
                - path           =    The llz file path
                - llz_header     =    LLZ_HEADER structure to be populated

 - Returns:
                - The file handle or -1 on error

********************************************************************/

int32_t open_llz (const char *path, LLZ_HEADER *llz_header)
{
  int32_t i, hnd, tf, uf;
  char varin[1024], info[1024], token[1024];


  /*  The first time through we want to initialize the llz handle array.  */

  if (first)
    {
      for (i = 0 ; i < MAX_LLZ_FILES ; i++) 
        {
          memset (&llzh[i], 0, sizeof (INTERNAL_LLZ_HEADER));
          llzh[i].fp = NULL;
          llz_recnum[i] = 0;
        }
      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  */

  hnd = MAX_LLZ_FILES;
  for (i = 0 ; i < MAX_LLZ_FILES ; i++)
    {
      if (llzh[i].fp == NULL)
        {
          hnd = i;
          llz_recnum[hnd] = 0;
          break;
        }
    }


  if (hnd == MAX_LLZ_FILES)
    {
      fprintf (stderr, "\n\nToo many open llz files!\n\n");
      return (-1);
    }


  /*  Open the file and read the header.  */

  tf = uf = 0;
  llzh[hnd].depth_units = 0;
  if ((llzh[hnd].fp = fopen64 (path, "rb+")) != NULL || (llzh[hnd].fp = fopen64 (path, "rb")) != NULL)
    {
      /*  We want to try to read the first line (version info) with an fread in case we mistakenly asked to
          load a binary file.  If we try to use ngets to read a binary file and there are no line feeds in 
          the first sizeof (varin) characters we would segfault.  */

      if (!fread (varin, 128, 1, llzh[hnd].fp)) return (-1);


      /*  Check for the version.  */

      if (!strstr (varin, "llz library V")) return (-1);


      /*  Rewind to the beginning of the file.  Yes, we'll read the version again but it doesn't matter.  */

      rewind (llzh[hnd].fp);


      while (ngets (varin, sizeof (varin), llzh[hnd].fp))
        {
          if (strstr (varin, "[END OF HEADER]")) break;


          /*  Put everything to the right of the equals sign in 'info'.   */

          if (strchr (varin, '=') != NULL) strcpy (info, (strchr (varin, '=') + 1));

          if (strstr (varin, "[VERSION]"))
            {
	      strcpy(llzh[hnd].header.version, info);
	      strcpy(token, strtok(info, "V"));
	      strcpy(token, strtok(NULL, info));
	      strcpy(token, strtok(token, " "));

	      llzh[hnd].major_version = (uint16_t) atoi(token);
            }

          if (strstr (varin, "[TIME FLAG]")) sscanf (info, "%d", &tf);

          if (strstr (varin, "[UNCERTAINTY FLAG]")) sscanf (info, "%d", &uf);

          if (strstr (varin, "[DEPTH UNITS]")) sscanf (info, "%c", &llzh[hnd].depth_units);

          if (strstr (varin, "[ENDIAN]"))
            {
              if (big_endian ())
                {
                  if (strstr (info, "LITTLE"))
                    {
                      llzh[hnd].swap = 1;
                    }
                  else
                    {
                      llzh[hnd].swap = 0;
                    }
                }
              else
                {
                  if (strstr (info, "BIG"))
                    {
                      llzh[hnd].swap = 1;
                    }
                  else
                    {
                      llzh[hnd].swap = 0;
                    }
                }
            }

          if (strstr (varin, "[CLASSIFICATION]")) strcpy (llzh[hnd].header.classification, info);

          if (strstr (varin, "[DISTRIBUTION]")) strcpy (llzh[hnd].header.distribution, info);

          if (strstr (varin, "[DECLASSIFICATION]")) strcpy (llzh[hnd].header.declassification, info);

          if (strstr (varin, "[CLASSIFICATION JUSTIFICATION]")) strcpy (llzh[hnd].header.class_just, info);

          if (strstr (varin, "[DOWNGRADE]")) strcpy (llzh[hnd].header.downgrade, info);

          if (strstr (varin, "[SOURCE]")) strcpy (llzh[hnd].header.source, info);

          if (strstr (varin, "[COMMENTS]")) strcpy (llzh[hnd].header.comments, info);

          if (strstr (varin, "[NUMBER OF RECORDS]")) sscanf (info, "%d", &llzh[hnd].header.number_of_records);

          if (strstr (varin, "[CREATION DATE]")) strcpy (llzh[hnd].header.creation_date, info);

          if (strstr (varin, "[LAST MODIFIED DATE]")) strcpy (llzh[hnd].header.modified_date, info);
        }

      llzh[hnd].time_flag = llzh[hnd].header.time_flag = (uint8_t) tf;
      llzh[hnd].uncertainty_flag = llzh[hnd].header.uncertainty_flag = (uint8_t) uf;
      llzh[hnd].depth_units = llzh[hnd].header.depth_units;
      llzh[hnd].at_end = 0;
      llzh[hnd].size_changed = 0;
      llzh[hnd].modified = 0;
      llzh[hnd].created = 0;
      llzh[hnd].write = 0;


      *llz_header = llzh[hnd].header;
    }
  else
    {
      hnd = -1;
    }


  return (hnd);
}


/********************************************************************/
/*!

 - Function:    close_llz

 - Purpose:     Close an llz file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/31/06

 - Arguments:   hnd            =    The file handle

 - Returns:     void

********************************************************************/

void close_llz (int32_t hnd)
{
  time_t systemtime;
  char time_date[128];

  systemtime = time (&systemtime);
  strcpy (time_date, asctime (localtime (&systemtime)));

  time_date[strlen (time_date) - 1] = 0;

  if (llzh[hnd].modified)
    {
      strcpy (llzh[hnd].header.modified_date, time_date);
    }

  if (llzh[hnd].created)
    {
      strcpy (llzh[hnd].header.creation_date, time_date);
    }

  if (llzh[hnd].size_changed || llzh[hnd].created || llzh[hnd].modified) write_llz_header (hnd);


  fclose (llzh[hnd].fp);
  llzh[hnd].fp = NULL;
  memset (&llzh[hnd], 0, sizeof (INTERNAL_LLZ_HEADER));
  llzh[hnd].fp = NULL;
}


/********************************************************************/
/*!

 - Function:    read_llz

 - Purpose:     Retrieve an llz record from an llz file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/31/06

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the llz
                                      record to be retrieved or
                                      LLZ_NEXT_RECORD (-1)
                - data           =    The returned llz record

 - Returns:
                - 0 on error or end of file
                - 1

********************************************************************/

uint8_t read_llz (int32_t hnd, int32_t recnum, LLZ_REC *data)
{
  int64_t pos;
  int32_t tmpi;
  INTERNAL_LLZ llz;


  /*  Flush the buffer if the last thing we did was a write operation.  */

  if (llzh[hnd].write) fflush (llzh[hnd].fp);


  /*  Set recnum for LLZ_NEXT_RECORD  */

  if (recnum < 0) 
    {
      if (llz_recnum[hnd] == llzh[hnd].header.number_of_records) return (0);
      recnum = llz_recnum[hnd];
    }


  /*  Version 1.00 files.  */

  if (llzh[hnd].major_version < 2)
    {
      pos = (int64_t) recnum * 4L * (int64_t) sizeof (int32_t) + (int64_t) LLZ_HEADER_SIZE;
      fseeko64 (llzh[hnd].fp, pos, SEEK_SET);

      llz.tv_sec = 0;
      llz.tv_nsec = 0;
      llz.uncertainty = 0.0;

      if ((fread (&llz.lat, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&llz.lon, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&llz.dep, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&tmpi, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);


      /*  Deal with the 16 bit v1.0 status.  */

      if (llzh[hnd].swap)
	{
	  swap_int (&tmpi);
	  llz.stat = (uint16_t) tmpi;
	  swap_int ((int32_t *) &llz.stat);
	}
      else
	{
	  llz.stat = (uint16_t) tmpi;
	}
    }


  /* Version > 2-3 files that were created during the 'default to version 0' bug */

  else if(llzh[hnd].major_version < 4 && llzh[hnd].major_version >= 2)
    {
      rewind(llzh[hnd].fp);

      pos = (int64_t) recnum * 4L * (int64_t) sizeof (int32_t) + (int64_t) LLZ_HEADER_SIZE;


      /*  all version above 1 have TIME FLAG  */

      if (llzh[hnd].time_flag)
	{
	  pos += (int64_t) recnum * (2L * (int64_t) sizeof (int32_t));
	}


      /*Version 3 has uncertainty*/

      if(llzh[hnd].major_version==3)
	{
	  if (llzh[hnd].uncertainty_flag)
	    {
	      pos += (int64_t) recnum * ((int64_t) sizeof (int32_t));
	    }
	}

      fseeko64 (llzh[hnd].fp, pos, SEEK_SET);

      llz.tv_sec = 0;
      llz.tv_nsec = 0;
      llz.uncertainty = 0.0;


      /* all version above 1 have TIME FLAG */      

      if (llzh[hnd].time_flag)
	{
	  if ((fread (&llz.tv_sec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	  if ((fread (&llz.tv_nsec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	}


      /* Version 3 has uncertainty */

      if (llzh[hnd].major_version==3)
	{
	  if (llzh[hnd].uncertainty_flag)
	    {
	      if ((fread (&llz.uncertainty, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) 
		return (0);
	    }
	}
      

      if ((fread (&llz.lat, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&llz.lon, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&llz.dep, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&tmpi, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);


      /*  Deal with the 16 bit v1.0 status.  */

      if (llzh[hnd].swap)
	{
	  swap_int (&tmpi);
	  llz.stat = (uint16_t) tmpi;
	  swap_int ((int32_t *) &llz.stat);
	}
      else
	{
	  llz.stat = (uint16_t) tmpi;
	}
    }


  /* Version > 4 using a uint16_t status instead of uint32_t */

  else
    {
      rewind(llzh[hnd].fp);

      pos = (int64_t) recnum * (3L * (int64_t) sizeof (int32_t) + (int64_t) sizeof (uint16_t)) + (int64_t) LLZ_HEADER_SIZE;

      if (llzh[hnd].time_flag)
	{
	  pos += (int64_t) recnum * (2L * (int64_t) sizeof (int32_t));
	}
	
      if (llzh[hnd].uncertainty_flag)
	{
	  pos += (int64_t) recnum * ((int64_t) sizeof (int32_t));
	}

      fseeko64 (llzh[hnd].fp, pos, SEEK_SET);

      llz.tv_sec = 0;
      llz.tv_nsec = 0;
      llz.uncertainty = 0.0;

      if (llzh[hnd].time_flag)
	{
	  if ((fread (&llz.tv_sec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	  if ((fread (&llz.tv_nsec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	}

      if (llzh[hnd].uncertainty_flag)
	{
	  if ((fread (&llz.uncertainty, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	}

      if ((fread (&llz.lat, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&llz.lon, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&llz.dep, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fread (&llz.stat, sizeof (uint16_t), 1, llzh[hnd].fp)) == 0) return (0);
    }


  /*  Set the next record number.  */

  llz_recnum[hnd]++;

  if (llzh[hnd].swap) swap_internal_llz (&llz);

  data->tv_sec = llz.tv_sec;
  data->tv_nsec = llz.tv_nsec;
  data->uncertainty = (float) llz.uncertainty / 10000.0L;
  data->xy.lat = (double) llz.lat / 10000000.0L;
  data->xy.lon = (double) llz.lon / 10000000.0L;
  data->depth = (float) llz.dep / 10000.0L;
  data->status = (uint32_t) llz.stat;

  llzh[hnd].at_end = 0;
  llzh[hnd].write = 0;

  return (1);
}


/********************************************************************/
/*!

 - Function:    append_llz

 - Purpose:     Store an llz record on the end of an llz file and update
                the number_of_records.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/31/06

 - Arguments:
                - hnd            =    The file handle
                - data           =    The llz record

 - Returns:
                - 0 on error
                - 1

********************************************************************/

uint8_t append_llz (int32_t hnd, LLZ_REC data)
{
  INTERNAL_LLZ llz;


  /*  Flush the buffer if the last thing we did was a read operation.  */

  if (!llzh[hnd].write) fflush (llzh[hnd].fp);


  if (!llzh[hnd].at_end) fseeko64 (llzh[hnd].fp, 0L, SEEK_END);


  llz.tv_sec = data.tv_sec;
  llz.tv_nsec = data.tv_nsec;
  llz.uncertainty = NINT (data.uncertainty * 10000.0L);
  llz.lat = NINT (data.xy.lat * 10000000.0L);
  llz.lon = NINT (data.xy.lon * 10000000.0L);
  llz.dep = NINT (data.depth * 10000.0L);
  llz.stat = (uint16_t) data.status;


  /*  Swap it if the file was originally swapped.  */

  if (llzh[hnd].swap) swap_internal_llz (&llz);


  if (llzh[hnd].time_flag)
    {
      if ((fwrite (&llz.tv_sec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.tv_nsec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
    }

  if (llzh[hnd].uncertainty_flag)
    {
      if ((fwrite (&llz.uncertainty, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
    }

  if ((fwrite (&llz.lat, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
  if ((fwrite (&llz.lon, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
  if ((fwrite (&llz.dep, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0); 


  /*  Version 1.00 files.  */

  if (llzh[hnd].major_version < 2)
    {
      if ((fwrite (&llz.stat, sizeof (uint32_t), 1, llzh[hnd].fp)) == 0) return (0);
    }


  /* Version > 2-3 files that were created during the 'default to version 0' bug */ 

  else if(llzh[hnd].major_version < 4 && llzh[hnd].major_version >= 2) 
    {
      if ((fwrite (&llz.stat, sizeof (uint32_t), 1, llzh[hnd].fp)) == 0) return (0);
    }


  /* Version > 4 using a uint16_t status instead of uint32_t */

  else
    {
      if ((fwrite (&llz.stat, sizeof (uint16_t), 1, llzh[hnd].fp)) == 0) return (0);
    }


  llzh[hnd].header.number_of_records++;
  llzh[hnd].size_changed = 1;
  llzh[hnd].modified = 1;
  llzh[hnd].write = 1;
  llzh[hnd].at_end = 1;

  return (1);
}


/********************************************************************/
/*!

 - Function:    update_llz

 - Purpose:     Store an llz record at the recnum record location in
                an llz file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/31/06

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number
                - data           =    The llz record

 - Returns:
                - 0 on error
                - 1

********************************************************************/

uint8_t update_llz (int32_t hnd, int32_t recnum, LLZ_REC data)
{
  int64_t pos;
  INTERNAL_LLZ llz;


  if (recnum > llzh[hnd].header.number_of_records - 1) return (0);


  /*  Flush the buffer if the last thing we did was a read operation.  */

  if (!llzh[hnd].write) fflush (llzh[hnd].fp);

  llz.tv_sec = data.tv_sec;
  llz.tv_nsec = data.tv_nsec;
  llz.uncertainty = NINT (data.uncertainty * 10000.0L);
  llz.lat = NINT (data.xy.lat * 10000000.0L);
  llz.lon = NINT (data.xy.lon * 10000000.0L);
  llz.dep = NINT (data.depth * 10000.0L);
  llz.stat = (uint16_t) data.status;


  /*  Swap it if the file was originally swapped.  */

  if (llzh[hnd].swap) swap_internal_llz (&llz);


  /*  Version 1.00 files.  */

  if (llzh[hnd].major_version < 2)
    {
      pos = (int64_t) recnum * 4L * (int64_t) sizeof (int32_t) + (int64_t) LLZ_HEADER_SIZE;
      fseeko64 (llzh[hnd].fp, pos, SEEK_SET);

      if ((fwrite (&llz.lat, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.lon, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.dep, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.stat, sizeof (uint32_t), 1, llzh[hnd].fp)) == 0) return (0);
    }


  /* Version > 2-3 files that were created during the 'default to version 0' bug */ 

  else if(llzh[hnd].major_version < 4 && llzh[hnd].major_version >= 2)
    {
      pos = (int64_t) recnum * 4L * (int64_t) sizeof (int32_t) + (int64_t) LLZ_HEADER_SIZE;


      /* all version above 1 have TIME FLAG */            

      if (llzh[hnd].time_flag) pos += (int64_t) recnum * (2L * (int64_t) sizeof (int32_t));


      /* verison3 has uncert */      

      if(llzh[hnd].major_version ==3)
	{
	  if (llzh[hnd].uncertainty_flag) pos += (int64_t) recnum * ((int64_t) sizeof (int32_t));
	}
      fseeko64 (llzh[hnd].fp, pos, SEEK_SET);


      /* all version above 1 have TIME FLAG */      

      if (llzh[hnd].time_flag)
	{
	  if ((fwrite (&llz.tv_sec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	  if ((fwrite (&llz.tv_nsec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	}


      /* verison3 has uncert */      

      if(llzh[hnd].major_version ==3)
	{
	  if (llzh[hnd].uncertainty_flag)
	    {
	      if ((fwrite (&llz.uncertainty, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	    }
	}

      if ((fwrite (&llz.lat, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.lon, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.dep, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.stat, sizeof (uint32_t), 1, llzh[hnd].fp)) == 0) return (0);
    }


  /* Version > 4 using a uint16_t status instead of uint32_t */

  else
    {
      pos = (int64_t) recnum * (3L * (int64_t) sizeof (int32_t) + (int64_t) sizeof (uint16_t)) + (int64_t) LLZ_HEADER_SIZE;

      if (llzh[hnd].time_flag) pos += (int64_t) recnum * (2L * (int64_t) sizeof (int32_t));
      if (llzh[hnd].uncertainty_flag) pos += (int64_t) recnum * ((int64_t) sizeof (int32_t));

      fseeko64 (llzh[hnd].fp, pos, SEEK_SET);

      if (llzh[hnd].time_flag)
	{
	  if ((fwrite (&llz.tv_sec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	  if ((fwrite (&llz.tv_nsec, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	}

      if (llzh[hnd].uncertainty_flag)
	{
	  if ((fwrite (&llz.uncertainty, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
	}

      if ((fwrite (&llz.lat, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.lon, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.dep, sizeof (int32_t), 1, llzh[hnd].fp)) == 0) return (0);
      if ((fwrite (&llz.stat, sizeof (uint16_t), 1, llzh[hnd].fp)) == 0) return (0);
    }

  llzh[hnd].modified = 1;
  llzh[hnd].write = 1;

  return (1);
}


/********************************************************************/
/*!

 - Function:    ftell_llz

 - Purpose:     Returns the location of the pointer within the LLZ file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/12/08

 - Arguments:   hnd            =    The file handle

 - Returns:     int32_t       =    location

********************************************************************/

int32_t ftell_llz (int32_t hnd)
{
  return (ftell (llzh[hnd].fp));
}
