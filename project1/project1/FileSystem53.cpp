//============================================================================
// Name        : FileSystem53.cpp
// Author      : Ford Tang, Ray Chou, Nick Imkamp, Michael Oh
// Version     :
// Copyright   : Your copyright notice
// Description : First Project Lab
//============================================================================

#include <iostream>
#include <string>
#include <fstream>
#include <exception>
using namespace std;

class FileSystem53 {

    int B;  //Block length
	int bytes;
	int K;  //Number of blocks for descriptor table
	int open_files;
	ofstream ofile;
	ifstream ifile;
	char** ldisk;
	char open_file_table[264];
	char** desc_table;  // Descriptor Table (in memory).
    // This is aka cache. It's contents should be
    // maintained to be same as first K blocks in disk.
	// Descriptor table format:
	// +-------------------------------------+
	// | bitmap | dsc_0 | dsc_1 | .. | dsc_N |
	// +-------------------------------------+
	//   bitmap: Each bit represent a block in a disk. MAX_BLOCK_NO/8 bytes
	//   dsc_0 : Root directory descriptor
	//   dsc_i : i'th descriptor. Each descriptor is FILE_SIZE_FIELD+ARRAY_SIZE bytes long.

	// Filesystem format parameters:
	static const int FILE_SIZE_FIELD = 1;     // Size of file size field in bytes. Maximum file size allowed in this file system is 192.
	static const int ARRAY_SIZE = 3;          // The length of array of disk block numbers that hold the file contents.
	static const int DESCR_SIZE = FILE_SIZE_FIELD+ARRAY_SIZE;
	static const int MAX_FILE_NO = 14;        // Maximum number of files which can be stored by this file system.
	static const int MAX_BLOCK_NO = 64;       // Maximum number of blocks which can be supported by this file system.
	static const int MAX_BLOCK_NO_DIV8 = MAX_BLOCK_NO/8;
	static const int MAX_FILE_NAME_LEN = 10;  // Maximum size of file name in byte.
	static const int MAX_OPEN_FILE = 3;       // Maximum number of files to open at the same time.
	static const int FILEIO_BUFFER_SIZE = 64; // Size of file io bufer
	static const int _EOF= -1;       // End-of-File

    public:

	/* Constructor of this File system.
		 *   1. Initialize IO system.
		 *   2. Format it if not done.
		 */
	FileSystem53(int l, int b, string storage)
		{
			B = l;
			bytes = b;
			K = 1;
			ldisk = new char*[l];
			for (int i = 0; i < B; i++)
			{
				ldisk[i] = new char[bytes];
			}
			format();
		}
		

	// Open File Table(OFT).
	void OpenFileTable()
	{
		for (int i = 0; i < 198; i++)
				open_file_table[i] = 0;
		open_file_table[0] = -1;
		open_file_table[1] = -1;
		open_file_table[66] = -1;
		open_file_table[67] = -1;
		open_file_table[132] = -1;
		open_file_table[133] = -1;
		open_file_table[198] = 0;
		open_file_table[199] = 0;
		char temp_block[64];
		read_block(2, temp_block);
		for (int i = 0; i < 64; i++)
			open_file_table[200 + i] = temp_block[i];
	}

	// Allocate open file table
	int find_oft()
	{
		for (int i = 0; i < MAX_OPEN_FILE; i++)
		{
			if (open_file_table[i * 66] == -1)
				return (i * 66);
		}
		return -1;
	}
	
	//Deallocate
	void deallocate_oft(int index)
	{
		for (int i = index; i < index + 66; i++)
		{
			if (i == index)
				open_file_table[i] = -1;
			else
				open_file_table[i] = 0;
		}
	}

	/* Format file system.
	 *   1. Initialize the first K blocks with zeros.
	 *   2. Create root directory descriptor for directory file.
	 * Parameter(s):
	 *   none
	 * Return:
	 *   none
	 */
	void format()
	{
		char temp_block[64];
		for (int i = 0; i < 64; i++)
			temp_block[i] = 0;
		for (int i = 0; i < B; i++)
			write_block(i, temp_block);
		for (int i = 0; i < 3; i++)
			temp_block[i] = 1;
		write_block(0, temp_block);
		OpenFileTable();
		char temp_descriptor[DESCR_SIZE] = {1, 2, 0, 0};
		write_descriptor(0, temp_descriptor);
	}


	/* Read descriptor
	 * Parameter(s):
	 *    no: Descriptor number to read
	 * Return:
	 *    Return char[4] of descriptor
	 */
	char* read_descriptor(int no)
	{
		char temp_block[64];
		read_block(1, temp_block);

		char result[4];
		for (int i = 0; i < DESCR_SIZE; i++)
			result[i] = temp_block[no + i];
		return result;
	}


	/* Clear descriptor
	 *   1. Clear descriptor entry
	 *   2. Clear bitmap
	 *   3. Write back to disk
	 * Parameter(s):
	 *    no: Descriptor number to clear
	 * Return:
	 *    none
	 */
	void clear_descriptor(int no)
	{
		char* temp_file_info;
		temp_file_info = read_descriptor(no);

		char temp_block[64];
		read_block(1, temp_block);

		for (int i = 0; i < DESCR_SIZE; i++)
			temp_block[no + i] = 0;

		write_block(1, temp_block);

		read_block(0, temp_block);

		for (int i = 1; i <= temp_file_info[0]; i++)
			temp_block[temp_file_info[i]] = 0;

		write_block(0, temp_block);
	}


	/* Write descriptor
	 *   1. Update descriptor entry
	 *   2. Mark bitmap
	 *   3. Write back to disk
	 * Parameter(s):
	 *    no: Descriptor number to write
	 *    desc: descriptor to write
	 * Return:
	 *    none
	 */
	void write_descriptor(int no, char* desc)
	{
		char temp_block[64];
		read_block(1, temp_block);

		for (int i = 0; i < DESCR_SIZE; i++)
			temp_block[no + i] = desc[i];

		write_block(1, temp_block);

		read_block(0, temp_block);

		for (int i = 1; i <= desc[0]; i++)
			temp_block[desc[i]] = 1;

		write_block(0, temp_block);
	}


	/* Search for an unoccupied descriptor.
	 * If ARRAY[0] is zero, this descriptor is not occupied.
	 * Then it returns descriptor number.
	 */
	int find_empty_descriptor()
	{
		char temp_block[64];
		read_block(1, temp_block);

		for (int i = 0; i < bytes / DESCR_SIZE; i++)
		{
			if (temp_block[i * DESCR_SIZE] == 0)
				return (i * DESCR_SIZE);
		}
		return -1;
	}


	/* Search for an unoccupied block.
	 *   This returns the first unoccupied block in bitmap field.
	 *   Return value -1 means all blocks are occupied.
	 * Parameter(s):
	 *    none
	 * Return:
	 *    Returns the block number
	 *    -1 if not found
	 */
	int find_empty_block()
	{
		char temp_block[64];
		read_block(0, temp_block);

		for (int i = 0; i < B; i++)
		{
			if (temp_block[i] == 0)
				return i;
		}
		return -1;
	}


	//Returns Open File Table Index of file descriptor index.
	int findex(int index)
	{
		if (index == 0)
			return 198;
		for (int i = 0; i < MAX_OPEN_FILE; i++)
		{
			if (index == open_file_table[i * 66])
			{
				return i * 66;
			}
		}
		return -1;
	}


	//Returns current position of file.
	int fcp(int index)
	{
		if (index == 0)
			return open_file_table[199];
		for (int i = 0; i < MAX_OPEN_FILE; i++)
		{
			if (index == open_file_table[i * 66])
			{
				return open_file_table[i * 66 + 1];
			}
		}
		return -1;
	}


	/* Get one character.
	 *    Returns the character currently pointed by the internal file position
	 *    indicator of the specified stream. The internal file position indicator
	 *    is then advanced to the next character.
	 * Parameter(s):
	 *    index (descriptor number of the file to be added.)
	 * Return:
	 *    On success, the character is returned.
	 *    If a writing error occurs, EOF is returned.
	 */
	int fgetc(int index)
	{
		int oft_index = findex(index);
		if (oft_index == -1)
			return _EOF;
		int current_position = fcp(index);
		if (current_position >= 192)
		{
			lseek(index, -1);
			return _EOF;
		}
		int current_block = (current_position / 64);
		int oft_file = oft_index / 66;
		int oft_position = current_position + (oft_file * 66) + 2 - (current_block * 64);
		open_file_table[oft_index + 1]++;
		int check_block = (fcp(index) / 64);
		char* temp_file_descriptor;
		temp_file_descriptor = read_descriptor(index);
		if (check_block > current_block)
		{
			if (temp_file_descriptor[check_block] != 0)
			{
				char temp_block[64];
				for (int i = 0; i < 64; i++)
					temp_block[i] = open_file_table[oft_index + 2 + i];
				temp_file_descriptor = read_descriptor(index);
				write_block(temp_file_descriptor[current_block], temp_block);
				read_block(temp_file_descriptor[check_block], temp_block);
				for (int i = 0; i < 64; i++)
					open_file_table[oft_index + 2 + i] = temp_block[i];
			}
			else
			{
				lseek(index, -1);
				return _EOF;
			}
		}
		return open_file_table[oft_position];


		/*for (int i = 0; i < MAX_OPEN_FILE; i++)
		{
			if (index == open_file_table[i * 66])
			{
				int current = open_file_table[(i * 66) + 1];
				int current_block = current / 64;
				current += (i * 66) + 2 - (current_block * 64);
				open_file_table[(i * 66) + 1]++;
				return open_file_table[current];
			}
		}
		return _EOF;*/
	}


	/* Put one character.
	 *    Writes a character to the stream and advances the position indicator.
	 *    The character is written at the position indicated by the internal position
	 *    indicator of the file, which is then automatically advanced by one.
	 * Parameter(s):
	 *    c: character to write
	 *    index (descriptor number of the file to be added.)
	 * Return:
	 *    On success, the character written is returned.
	 *    If a writing error occurs, -2 is returned.
	 */
	int fputc(int c, int index)
	{
		int oft_index = findex(index);
		if (oft_index == -1)
			return -2;
		int current_position = fcp(index);
		if (current_position >= 192)
			return -2;
		int current_block = current_position / 64;
		int oft_file = oft_index / 66;
		int oft_position = current_position + (oft_file * 66) + 2 - (current_block * 64);
		open_file_table[oft_position] = c;
		open_file_table[oft_index + 1]++;
		int check_block = fcp(index) / 64;
		if (check_block > current_block)
		{
			char temp_block[64];
			for (int i = 0; i < 64; i++)
				temp_block[i] = open_file_table[oft_index + 2 + i];
			char* temp_file_descriptor;
			temp_file_descriptor = read_descriptor(index);
			write_block(temp_file_descriptor[current_block], temp_block);
			read_block(temp_file_descriptor[check_block], temp_block);
			for (int i = 0; i < 64; i++)
				open_file_table[oft_index + 2 + i] = temp_block[i];
		}
		return c;
		/*for (int i = 0; i < MAX_OPEN_FILE; i++)
		{
			if (index == open_file_table[i * 66])
			{
				int current = open_file_table[(i * 66) + 1];
				open_file_table[current] = c;
				open_file_table[(i * 66) + 1]++;
				return c;
			}
		}
		return -2;*/
	}


	/* Check for the end of file.
	 * Parameter(s):
	 *    index (descriptor number of the file to be added.)
	 * Return:
	 *    Return true if end-of-file reached.
	 */
	bool feof(int index)
	{
		int oft_index = findex(index);
		if (oft_index == -1)
			return true;
		int current_position = fcp(index);
		if (current_position >= 192 || current_position == -1)
			return true;
		return false;
		/*for (int i = 0; i < MAX_OPEN_FILE; i++)
		{
			if (index == open_file_table[i * 66])
			{
				if (open_file_table[(i * 66) + 1] >= 191)
					return true;
			}
		}
		return false;*/
	}


	/* Search for a file
	 * Parameter(s):
	 *    index: index of open file table
	 *    st: The name of file to search.
	 * Return:
	 *    index: An integer number position of found file entry.
	 *    Return -1 if not found.
	 */
	int search_dir(int index, string symbolic_file_name)
	{
		char* temp_file_descriptor;
		temp_file_descriptor = read_descriptor(0);
		bool found = false;

		for (int a = 1; a <= temp_file_descriptor[0]; a++)
		{
			char temp_block[64];
			read_block(temp_file_descriptor[a], temp_block);

			for (int b = 0; b < 64; b++)
				open_file_table[index + 2 + b] = temp_block[b];
			open_file_table[index + 1] = (a - 1) * 64;
			for (int i = index + 2; i < index + 66; i++)
			{
				if (found)
						return open_file_table[i + symbolic_file_name.length()];
				if (open_file_table[i] == symbolic_file_name.length())
				{
					for (int j = 0; j < symbolic_file_name.length(); j++)
					{
						if (symbolic_file_name[j] != open_file_table[i + 1 + j])
						{
							found = false;
							break;
						}
						else
						{
							found = true;
							//return open_file_table[i + symbolic_file_name.length() + 1];  //returns index of file descriptor.
						}
					}
				}
			}
		}
		return -1;
	}


	/* Clear a file entry from directory file
	 *
	 * Parameter(s):
	 *    index: open file table index
	 *    start_pos:
	 *    length:
	 * Return:
	 *    none
	 */
	void delete_dir(string symbolic_file_name)
	{
		char* temp_file_descriptor;
		temp_file_descriptor = read_descriptor(0);
		bool found = false;
		int start_pos;
		int block;
		char temp_block[64];
		for (int a = 1; a <= temp_file_descriptor[0]; a++)
		{
			block = a;
			if (found)
				break;
			
			read_block(temp_file_descriptor[a], temp_block);

			for (int b = 0; b < 64; b++)
				open_file_table[200 + b] = temp_block[b];
			open_file_table[199] = (a - 1) * 64;
			for (int i = 200; i < 264; i++)
			{
				if (found)
					break;
				if (open_file_table[i] == symbolic_file_name.length())
				{
					for (int j = 0; j < symbolic_file_name.length(); j++)
					{
						if (symbolic_file_name[j] != open_file_table[i + 1 + j])
						{
							found = false;
							break;
						}
						else
						{
							found = true;
							start_pos = i;
						}
					}
				}
			}
		}
		for (int i = 0; i < symbolic_file_name.length() + 2; i++)
			open_file_table[start_pos + i] = 0;

		for (int b = 0; b < 64; b++)
				temp_block[b] = open_file_table[200 + b];

		temp_file_descriptor = read_descriptor(0);
		write_block(temp_file_descriptor[block],temp_block);
	}


	/* File creation function:
	 *    1. creates empty file with file size zero.
	 *    2. makes/allocates descriptor.
	 *    3. updates directory file.
	 * Parameter(s):
	 *    symbolic_file_name: The name of file to create.
	 * Return:
	 *    Return 0 for successful creation.
	 *    Return -1 for error (no space in disk)
	 *    Return -2 for error (for duplication)
	 */
	int create(string symbolic_file_name)
	{
		int found_name = search_dir(198, symbolic_file_name);
		if (found_name != -1)
			return -2;
		int empty_descriptor = find_empty_descriptor();
		if (empty_descriptor == -1)
			return -1;
		char temp_descriptor[DESCR_SIZE] = {0, 0, 0, 0};
		write_descriptor(empty_descriptor, temp_descriptor);

		char* temp_file_descriptor;
		temp_file_descriptor = read_descriptor(0);

		char temp_block[64];
		read_block(temp_file_descriptor[1], temp_block);

		for (int i = 0; i < 64; i++)
			open_file_table[200 + i] = temp_block[i];
		open_file_table[199] = 0;

		while (feof(0) == false)  //error
		{
			bool found_space = true;
			for (int i = 0; i < symbolic_file_name.length() + 2; i++)
			{
				int check = fgetc(0);
				if (check != 0)
				{
					found_space = false;
					break;
				}
			}
			if (found_space)
			{
				open_file_table[199] -= symbolic_file_name.length() + 1;
				break;
			}
		}

		if (feof(0))
			return -1;
		int current = open_file_table[199];
		open_file_table[199] = current - symbolic_file_name.length() + 2;

		for (int i = 0; i < symbolic_file_name.length() + 2; i++)
		{
			if (i == 0)
				fputc(symbolic_file_name.length(), 0);
			else if (i == symbolic_file_name.length() + 1)
				fputc(empty_descriptor, 0);
			else
				fputc(symbolic_file_name[i - 1], 0);
		}

		for (int i = 0; i < 64; i++)
			temp_block[i] = open_file_table[200 + i];

		temp_file_descriptor = read_descriptor(0);
		write_block(temp_file_descriptor[(current / 64) + 1], temp_block);

		return 0;
	}


	/* Open file with descriptor number function:
	 * Parameter(s):
	 *    desc_no: descriptor number
	 * Return:
	 *    index: index if open file table if successfully allocated.
	 *    Return -1 for error.
	 */
	int open_desc(int desc_no)
	{
		if (desc_no == open_file_table[0] || desc_no == open_file_table[66] || desc_no == open_file_table[132])
			return -1;
		int open_file_index = find_oft();
		if (open_file_index != -1)
		{
			char temp_block[64];
			read_block(desc_no, temp_block);

			for (int i = 0; i < bytes; i++)
			{
				open_file_table[open_file_index + 2 + i] = temp_block[i];
			}

			open_file_table[open_file_index] = desc_no;
			open_file_table[open_file_index + 1] = 0;
		}

		return open_file_index;
	}


	/* Open file with file name function:
	 * Parameter(s):
	 *    symbolic_file_name: The name of file to open.
	 * Return:
	 *    index: An integer number, which is a index number of open file table.
	 *    Return -1 or -2 if it cannot be open.
	 */
	// TODOs:
			// 1. Open directory file
			// 2. Search for a file with given name
			//    Return -1 if not found.
			// 3. Get descriptor number of the found file
			// 4. Looking for unoccupied entry in open file table.
			//    Return -2 if all entry are occupied.
			// 5. Initialize the entry (descriptor number, current position, etc.)
			// 6. Return entry number
	int open(string symbolic_file_name)
	{
		int found_name = search_dir(198, symbolic_file_name);
		if (found_name == -1)
			return -1;

		int open_file = open_desc(++found_name);
		if (open_file==-1)
			return -2;
		return open_file;
	}


	/* File Read function:
	 *    This reads a number of bytes from the the file indicated by index.
	 *    Reading should start from the point pointed by current position of the file.
	 *    Current position should be updated accordingly after read.
	 * Parameter(s):
	 *    index: File index which indicates the file to be read.
	 *    mem_area: buffer to be returned
	 *    count: number of byte(s) to read
	 * Return:
	 *    Actual number of bytes returned in mem_area[].
	 *    -1 value for error case "File hasn't been open"
	 *    -2 value for error case "End-of-file"
	 TODOs:
			 1. Read the open file table using index.
			    1.1 Get the file descriptor number and the current position.
			    1.2 Can't get proper file descriptor, return -1.
			 2. Read the file descriptor
			    2.1 Get file size and block array.
			 3. Read 'count' byte(s) from file and store in mem_area[].
			    3.1 If current position crosses block boundary, call read_block() to read the next block.
			    3.2 If current position==file size, stop reading and return.
			    3.3 If this is called when current position==file size, return -2.
			    3.4 If count > mem_area size, only size of mem_area should be read.
			    3.5 Returns actual number of bytes read from file.
			    3.6 Update current position so that next read() can be done from the first byte haven't-been-read.
    */
	int read(int index, char* mem_area, int count)
	{
		int found = 0;
		int getresult;
		for (int i=0;i<count; i++)
		{
			getresult = fgetc(index);
			if (getresult != _EOF)
			{
				mem_area[i] = getresult;
				found += 1;
			}
			else
				break;			
		}
			
		if (found == 0)
		{
			return -1;
		}
		else
		{
			return found;
		}

	}


	/* File Write function:
	 *    This writes 'count' number of 'value'(s) to the file indicated by index.
	 *    Writing should start from the point pointed by current position of the file.
	 *    Current position should be updated accordingly.
	 * Parameter(s):
	 *    index: File index which indicates the file to be read.
	 *    value: a character to be written.
	 *    count: Number of repetition.
	 * Return:
	 *    >0 for successful write
	 *    -1 value for error case "File hasn't been open"
	 *    -2 for error case "Maximum file size reached" (not implemented.)
	 */
	int write(int index, char value, int count)
	{
		int found = -1;
		for (int i = 0; i < MAX_OPEN_FILE; i++)
		{
			if (index == open_file_table[i * 66])
			{
				found=i*66;
				for (int i=0;i<count; i++)
				{
					fputc(value,index);
				}
				break;
			}
		}
		if (found==-1)
		{
			return found;
		}
		else
		{
			return 0;
		}
	}

	/* Setting new read/write position function:
	 * Parameter(s):
	 *    index: File index which indicates the file to be read.
	 *    pos: New position in the file. If pos is bigger than file size, set pos to file size.
	 * Return:
	 *    0 for successful write
	 *    -1 value for error case "File hasn't been open"
	 */
	int lseek(int index, int pos)
	{
		for (int i = 0; i < MAX_OPEN_FILE; i++)
		{
			if (index == open_file_table[i * 66])
			{
				open_file_table[i * 66 + 1] = pos;
				return 0;
			}
		}
		return -1;
	}


	/* Close file function:
	 * Parameter(s):
	 *    index: The index of open file table
	 * Return:
	 *    none
	 */
	void close(int index)
	{
		int fd_index = open_file_table[index];
		int current_position = open_file_table[index + 1];
		int current_block = current_position / 64;
		char temp_block[64];
		for (int i = 0; i < 64; i++)
			temp_block[i] = open_file_table[index + 2 + i];
		char* temp_file_descriptor;
		temp_file_descriptor = read_descriptor(fd_index);
		write_block(temp_file_descriptor[current_block], temp_block);
		for (int i = index; i < index + 66; i++)
			open_file_table[i] = 0;
	}


	/* Delete file function:
	 *    Delete a file
	 * Parameter(s):
	 *    symbolic_file_name: a file name to be deleted.
	 * Return:
	 *    Return 0 with success
	 *    Return -1 with error (ie. No such file).
	 */
	int deleteFile(string fileName)
	{
		int file_descriptor = search_dir(198, fileName);
		if (file_descriptor == -1)
			return -1;
		clear_descriptor(file_descriptor);
		delete_dir(fileName);

		return 0;
	}


	/* Directory listing function:
	 *    List the name and size of files in the directory. (We have only one directory in this project.)
	 *    Example of format:
	 *       abc 66 bytes, xyz 22 bytes
	 * Parameter(s):
	 *    None
	 * Return:
	 *    None
	 */
	void directory()
	{
		char* temp_descriptor;
		temp_descriptor = read_descriptor(0);
		char temp_block[64];
		read_block(temp_descriptor[1], temp_block);
		for (int b = 0; b < 64; b++)
			open_file_table[200 + b] = temp_block[b];
		open_file_table[199] = 0;
		string filename;
		int oft_dir_cp = open_file_table[199];
		while (!feof(0))
		{
			filename.clear();
			int len = fgetc(0);
			if (len == -1)
				break;
			else if (len != 0)
			{
				for (int i = 0; i < len; i++)
					filename += fgetc(0);
				int file_index = fgetc(0);
				
				int openresult = open_desc(file_index);
				if (openresult == -1)
					lseek(file_index, 0);

				char temp[192];

				int readcount = read(file_index, temp, 192);

				int count = 0;

				for (int i = 0; i <= readcount; i++)
				{
					if (temp[i] != -52 && temp[i] != 0)
						count++;
				}

				if (file_index == open_file_table[0])
					close(0);
				else if (file_index == open_file_table[66])
					close(66);
				else
					close(132);

				cout << filename << " " << count << " bytes" << endl;
			}
		}
		open_file_table[199] = oft_dir_cp;
	}


	/*------------------------------------------------------------------
	  Disk management functions.
	  These functions are not really a part of file system.
	  They are provided for convenience in this emulated file system.
	  ------------------------------------------------------------------
	 Restores the saved disk image in a file to the array.
	 */
	void restore()
	{
		string line;
		ifile.open("ldisk.txt");

		/*ifile >> line;
		while (!ifile.eof())
		{
			cout << line << endl;
			ifile >> line;
		}*/
		for (int i = 0; i < B; i++)
		{
			ifile >> line;
			for (int j = 0; j < bytes; j++)
			{
				ldisk[i][j] = line[j];
			}
		}
		ifile.close();
	}

	// Saves the array to a file as a disk image.
	void save()
	{
		ofile.open("ldisk.txt");
		for (int i = 0; i < B; i++)
		{
			for (int j = 0; j < bytes; j++)
			{
				ofile << ldisk[i][j];
			}
			ofile << endl;
		}
		ofile.close();
	}

	// Disk dump, from block 'start' to 'start+size-1'.
	void diskdump(int start, int size);

	// Copies 1 block from block number i in ldisk to memory at address p
	void read_block(int i, char *p)
		{
			for (int j = 0; j < bytes; j++)
			{
				p[j] = ldisk[i][j];
			}
		}
	
	// Copies 1 block starting at memory address p to ldisk block number I
	void write_block(int i, char *p)
		{
			for (int j = 0; j < bytes; j++)
			{
				ldisk[i][j] = p[j];
			}
		}


	void in(string diskfile)
	{
		/*try
		{
			cout << "Trying to restore " << diskfile << endl;
			restore();
			cout << "disk restored" << endl;
		}*/

		/*catch(exception& e)*/
		{
			format();
			cout << "disk initialized" << endl;
		}
	}


	void cr(string filename)
	{
		int result = create(filename);
		if (result == 0)
			cout << "file " << filename << " created" << endl;
		else if(result == -1)
			cout << "error:  no space for " << filename << endl;
		else
			cout << "error:  " << filename << " already exists" << endl;
	}


	void de(string filename)
	{
		int result = deleteFile(filename);
		if (result == 0)
			cout << "file " << filename << " destroyed" << endl;
		else
			cout << "error:  "<< filename << " does not exist" << endl;
	}


	void op(string filename)
	{
		int fd_index = search_dir(198, filename);
		if (fd_index == -1)
			cout << "error:  " << filename << " does not exist" << endl;
		else
		{
			int oft_index = open_desc(fd_index);
			if (oft_index == -1)
				cout << "error:  " << filename << " could not be opened" << endl;
			else
				cout << "file " << filename << " opened, index = " << oft_index << endl;
		}
		
	}


	void cl(int index)
	{
		close(index);
		cout << "file closed" << endl;
	}



	void rd(int index, int count) //check
	{
		int fd_index = open_file_table[index];
		char result[64];
		int read_status = read(fd_index, result, count);
		if (read_status == -1)
			cout << "error:  file not opened" << endl;
		else if (read_status == -2)
			cout << "error:  end of file" << endl;
		else
		{
			cout << read_status << " bytes read:  ";
			for (int i = 0; i < read_status; i++)
				cout << result[i];
			cout << endl;
		}
	}


	void wr(int index, char c, int count)
	{
		int fd_index = open_file_table[index];
		int write_status = write(fd_index, c, count);
		if (write_status == -1)
			cout << "error:  file has not been opened" << endl;
		else
			cout << count << " bytes written" << endl;

	}


	void sk(int index, int pos)
	{
		int fd_index = open_file_table[index];
		lseek(fd_index, pos);
	}


	void dr()
	{
		directory();
	}


	void sv(string filename)
	{
		save();
		cout << "disk saved" << endl;
	}
};

int main()
{
	FileSystem53 ldisk(64, 64, "ldisk");
	string a = "hello";
	ldisk.in(a);
	a = "foo";
	ldisk.cr(a);
	ldisk.op(a);
	ldisk.wr(0, 'a' , 10);
	ldisk.sk(0, 0);
	ldisk.rd(0, 10);
	ldisk.directory();
	ldisk.cl(0);
	ldisk.de(a);

	return 0;
}