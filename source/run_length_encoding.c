///////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////Run-length encoding (RLE)//////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#define FREQUENCY_FLAG 0
#define SYMBOL_FLAG 1

void int_to_print(int);
void char_to_print(char);
void write_codes(FILE*, unsigned char*, int*, int, int);
unsigned char read_codes(FILE* , unsigned char*, int*, int);

int main(int argc, char *argv[]) 
{
	unsigned char code_buffer; 
	unsigned int buffer_position = 0;
	unsigned char *file_buffer;
	
	unsigned char repeat_counter = 1;
	unsigned int file_size;
	unsigned int index;
	unsigned char current_symbol; 
	unsigned char previous_symbol;

	char *file_to_compress_path;

	FILE *file_to_compress_ptr; 
	FILE *file_w_codes_ptr; 
	FILE *decompressed_file_ptr;
	
	if (argc < 2)
	{
		printf("There is not enough arguments. Please file path (must be not longer than 128 symbols).");
		return -1;
	}

	file_to_compress_path = argv[1];

	file_to_compress_ptr = fopen(file_to_compress_path, "rb");

	if(file_to_compress_ptr==NULL) //check if file is opened
    {
        printf("Error: File could not be opened.");
        return -1;
    }

	file_w_codes_ptr = fopen("codes", "wb");
	if (file_w_codes_ptr == NULL) 
	{
		printf("Error: File could not be opened.");
		fclose(file_to_compress_ptr);
		return -1;
	}

	fseek(file_to_compress_ptr, 0, SEEK_END); // seek to end of file
	file_size = ftell(file_to_compress_ptr); // get current file pointer
	fseek(file_to_compress_ptr, 0, SEEK_SET); // seek back to beginning of file
	
	if (!(file_buffer = (unsigned char*)malloc(file_size)))
	{
		printf("Error: Cannot allocate memory.");
		fclose(file_to_compress_ptr);
		fclose(file_w_codes_ptr);
		return -1;
	}
		
	fread(file_buffer, sizeof(char), file_size, file_to_compress_ptr);
	
	fwrite(&file_size, sizeof(int), 1, file_w_codes_ptr);

	previous_symbol = file_buffer[0];

	for(index = 1; index <= file_size; ++index)
	{
		if (index < file_size) current_symbol = file_buffer[index];
		if (current_symbol == previous_symbol && repeat_counter < 255 && index != file_size) repeat_counter++;
		else
		{
			if (repeat_counter > 1)
			{
				write_codes(file_w_codes_ptr, &code_buffer, &buffer_position, FREQUENCY_FLAG, 1);
				write_codes(file_w_codes_ptr, &code_buffer, &buffer_position, repeat_counter, 8);
			}
			else write_codes(file_w_codes_ptr, &code_buffer, &buffer_position, SYMBOL_FLAG, 1);
			write_codes(file_w_codes_ptr, &code_buffer, &buffer_position, previous_symbol, 8);
			previous_symbol = current_symbol;
			repeat_counter = 1;
		}
	}
	
	if (buffer_position > 0)
	{
		code_buffer = code_buffer << (8 - buffer_position);
		fwrite(&code_buffer, sizeof(char), 1, file_w_codes_ptr);
	}

	
	fclose(file_to_compress_ptr);
	fclose(file_w_codes_ptr);
	free(file_buffer);

	////////////////////////////////////////////////////////////
	////Decompression///////////////////////////////////////////
	////////////////////////////////////////////////////////////
	{
		unsigned int read_data_counter = 0;
		char flag; 
		code_buffer = 0;
		buffer_position = 0;

		file_w_codes_ptr = fopen("codes", "rb");
		decompressed_file_ptr = fopen("decompressed", "wb");

		fread(&file_size, sizeof(int), 1, file_w_codes_ptr);

		while (read_data_counter < file_size)
		{
			flag = read_codes(file_w_codes_ptr, &code_buffer, &buffer_position, 1);
			if (flag == SYMBOL_FLAG)
			{
				current_symbol = read_codes(file_w_codes_ptr, &code_buffer, &buffer_position, 8);
				fwrite(&current_symbol, sizeof(char), 1, decompressed_file_ptr);
				++read_data_counter;
			}

			else
			{
				repeat_counter = read_codes(file_w_codes_ptr, &code_buffer, &buffer_position, 8);
				current_symbol = read_codes(file_w_codes_ptr, &code_buffer, &buffer_position, 8);
				for (; repeat_counter > 0; --repeat_counter, ++read_data_counter) fwrite(&current_symbol, sizeof(char), 1, decompressed_file_ptr);
			}
		}

		fclose(file_w_codes_ptr);
		fclose(decompressed_file_ptr);
	}

	system("pause");	
	return 0;
}


void int_to_print(int value_to_print)
{
	
	int i;
	for(i = (8 * sizeof(int)) - 1; i >=0 ; --i)
	{
		if ((value_to_print & (1 << i)) != 0) printf("1");
		else printf("0");
	}
	printf("\n");
}

void char_to_print(char value_to_print)
{

	int i;
	for (i = (8 * sizeof(char)) - 1; i >= 0; --i)
	{
		if ((value_to_print & (1 << i)) != 0) printf("1");
		else printf("0");
	}
	printf("\n");
}

unsigned char read_codes(FILE *file_w_codes_ptr, unsigned char *buffer, int *buffer_position, int data_length)
{
	int code;

	if (*buffer_position >= data_length)
	{
		code = *buffer >> (*buffer_position - data_length);
		*buffer = *buffer & (~(code << (*buffer_position - data_length)));
		*buffer_position = *buffer_position - data_length;
		return code;
	}
	else
	{
		int transfer_code_length = data_length - *buffer_position;
		int transfer_code = *buffer << transfer_code_length;
		fread(buffer, sizeof(char), 1, file_w_codes_ptr);
		*buffer_position = 8;

		return transfer_code | read_codes(file_w_codes_ptr, buffer, buffer_position, transfer_code_length);
	}
	
}

void write_codes(FILE *file_w_codes_ptr, unsigned char *buffer, int *buffer_position, int code, int code_length)
{
	while(code_length > 0) 
	{
		if ((*buffer_position + code_length) < 8)
		{
		*buffer = *buffer << code_length;
		*buffer = *buffer | code;
		*buffer_position = *buffer_position + code_length;
		code_length = 0;
		}

		else
		{
			int transfer_code_length = (sizeof(*buffer)* 8 - *buffer_position);
			int transfer_code = code >> (code_length - transfer_code_length);

			*buffer = *buffer << transfer_code_length;
			*buffer = *buffer | transfer_code;
		
			fwrite(buffer, sizeof(char), 1, file_w_codes_ptr);
			*buffer_position = 0;
			code_length -= transfer_code_length;
			code = code & (~(transfer_code << (code_length)));
		}
	}	
}
