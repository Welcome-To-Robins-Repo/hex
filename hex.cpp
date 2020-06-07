#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <charconv>

#include <termios.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define unsaved "Buffer has unsaved changes!"
#define empty "Buffer is empty!"
#define eof "Offset is at the End of File!"
#define nowinsize "Couldn't get window size!"
#define small "Window is too small!"
#define rerr "Read error!"
#define terr "Couldn't set terminal attributes!"
#define cerr "Bad command formatting!"

struct clip;
struct mark;
struct file_buffer
{
	//buffer
	std::string b;
	//pathname
	std::string p;
	//edited flag
	bool e = false;
	//offset
	int o = 0;
	//clipboard for the file
	std::vector<clip>c;
	//volatile clipboard for unnamed y and d operations
	std::string v;
	//list of marked addresses
	std::vector <mark>m;
};
struct clip
{
	//buffer
	std::string b;
	//buffer name
	std::string n;
};
struct mark
{
	char n[4];
	int o;
};

//terminal states
termios ui, preserve;

//points to file being edited
file_buffer *buffer;
//list of files given in arguments
std::vector<file_buffer> files;

//function to restore terminal and terminate hex
void restore ();
//function to add bytes to buffer
std::string getText ();
//function to convert hexadecimal string to character value
char toC (char *h);
//function to convert character to hexadecimal string
char *toH (char c);
//function to print an offset
void printo ();
//overload of printo for printing a buffer that's not the current buffer
void printo (int b);
//convert hex address to decimal
int htoa (char *h);
//refegex find
int rfind (std::string r);
//regex replace
void rrplace (std::string r);
//returns a string of input
std::string gin ();

//data for undo command
std::string ub; //holds buffer of last bytes added or removed
int uo, ul; //holds the (o)ffset the last command acted on and the (l)ength of bytes
bool ua; //true if the last operation added bytes, false if bytes were removed


//variables for the state machine thingy that parses commands
int o[3], ac = 0, rc = 0, s = 0, count = 0;
int confirm = 0;
/* marks the type of command parsed
* each address is +1
* command is +3
*/
char 	cmd = 0,
	opt = 0,
	a[8] = {0,0,0,0,0,0,0,0},
	name[4] = {0,0,0,0},
	key = 0,
	keyword[4] = {0,0,0,0},
	mk[4] = {0,0,0,0};
std::string rx[2], path, prx[2], in;
int values [8] = {268435456,16777216,1048576,65536,4096,256,16,1};

void layer1 (char k);
void layer2 ();
void execute ();

int main (int argc, char **argv)
{
	int current_buffer = 0, pos;
	std::string c, tmp;
	char g;
	std::fstream fs;
	std::stringstream sts, t;
	winsize w;

	if (argc > 1)
	{
		for (int i = 1; i < argc; i ++)
		{
			files.emplace_back ();
			files.back ().p = argv[i];
			fs.open (argv[i], std::ios_base::binary | std::ios_base::in);
			if (fs.is_open ())
			{
				sts << fs.rdbuf ();
				fs.close ();
				files.back ().b.assign (sts.str ());
			}
			else
			{
				fs.close ();
				files.back ().b.clear ();
			}
		}
	}
	else
	{
		files.emplace_back ();
	}

	buffer = &files.front ();

	tcgetattr (0, &preserve);
	ui = preserve;
	ui.c_iflag  &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	//ui.c_oflag &= ~OPOST;
	ui.c_lflag  &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	ui.c_cflag &= ~(CSIZE | PARENB);
	ui.c_cflag |= CS8;
	tcsetattr (0, TCSANOW, &ui);

	ioctl (0, TIOCGWINSZ, &w);
	for (int i = 0; i < w.ws_row; i ++)
		t << '\n';
	write (1, t.str ().c_str (), t.str ().size ());

	/*
	* simple state machine that parses the input, populates the command
	* structure variables, and executes the specified commands.
	* s specifies the current state:
	* -1: error state, continue to next command
	* 0: init
	* 1: aggregate an address
	* 2: aggregate a command
	* 3: aggregate a name
	* 4: aggregate a mark
	* 5: aggregate an option
	* 6: regular expression search address
	* 7: aggregate a regular expression
	* 8: aggregate keyword command
	* 9: aggregate pathname
	*/
	bool addr = false, succ = false, r = true;
	std::stringstream out;
	while (1)
	{
		t.str (gin ());

		while (t.get(key))
		{
			switch (s)
			{
				case -1:
				{
					switch (key)
					{
						case '\n':
						case '\r':
						case '|':
						{
							s = 0;
							break;
						}
					}
					break;
				}
				case 0: //init
				{
					switch (key)
					{
						case '[':
						{
							s = 1;
							break;
						}
						case 'a': break;
						case 'c': break;
						case 'i': break;
						{
							cmd = key;
							confirm += 3;
							break;
						}
						case 'd': break;
						case 'y': break;
						case 'v': break;
						case 'z': break;
						case 'm': break;
						case 'p': break;
						case 'x': break;
						case 'u': break;
						case 'r': break;
						case 'o': break;
						case 's': break;
						case 'n': break;
						case 'w': break;
						case 'q':
						{
							cmd = key;
							s = 5;
							confirm += 3;
							break;
						}
						case 'f': break;
						case '*': break;
					}
					break;
				}
				case 1: //aggregate an address
				{
					switch (key)
					{
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
						case 'a':
						case 'b':
						case 'c':
						case 'd':
						case 'e':
						case 'f':
						{
							if (count < 8)
							{
								a[count] = key;
								count ++;
							}
							else
							{
								write (1, "?", 1);
								s = -1;
							}
							break;
						}
						case ']':
						{
							if (count == 0)
							{
								write (1, "?", 1);
								s = -1;
								break;
							}
							int tmp = 7;
							for (int i = count - 1; i > -1; i --)
							{
								if (a[i] > 57)
								{
									o[ac] += (a[i] - 87) * values[tmp];
								}
								else
								{
									o[ac] += (a[i] - 48) * values[tmp];
								}
								tmp --;
							}
							confirm += 1;
							if (ac < 2)
							{
								s = 0;
							}
							else
							{
								s = 2;
							}
							break;
						}
						default:
						{
							write (1, "?", 1);
							s = -1;
							break;
						}
					}
					break;
				}
				case 2: //aggregate a command
				{
					if (confirm >= 3)
					{
						s = -1;

					}
					break;
				}
				case 5: //aggregate an option
				{
					switch (key)
					{
						case '!':
						{
							opt = key;

						}
					}
					break;
				}
			}
		}
		t.str ("");
		t.clear ();
		in.clear ();
		write (1, "\r\x1b[2K", 5);
		switch (confirm)
		{
			case 0:
			{
				printo();
				if (buffer->o + 16 < buffer->b.size () + 1)
					buffer->o += 16;
				break;
			}
			case 1:
			{
				buffer->o = o[0];
				printo (o[0]);
				break;
			}
			case 2: break;
			case 3:
			{
				switch (cmd)
				{
					case 'q':
					{
						if (buffer->e and opt == '!')
							restore ();
						else if (!buffer->e)
							restore ();
						else
							write (1, "!", 1);
					}
				}
			}
			case 4: break;
			case 5: break;
		}
		write (1, "\n", 1);
		o[0] = 0;
		o[1] = 0;
		o[2] = 0;
		ac = 0;
		a[0] = 0;
		a[1] = 0;
		a[2] = 0;
		a[3] = 0;
		a[4] = 0;
		a[5] = 0;
		a[6] = 0;
		a[7] = 0;
		count = 0;
		confirm = 0;
		cmd = 0;

	}
	return 0;
}

void restore ()
{
	tcsetattr (0, TCSANOW, &preserve);
	write (1, "\n", 1);
	exit (0);
}

char toC (char *h)
{
	char r = 0;
	if (h[1] > 57)
		r += h[1] - 87;
	else
		r += h[1] - 48;
	if (h[0] > 57)
		r += (h[0] - 87) * 16;
	else
		r += (h[0] - 48) * 16;
	return r;
}

char * toH (char c)
{
	char *x = new char[3];
	x[0] = c / 16;
	x[1] = c % 16;
	if (x[0] > 9)
		x[0] += 87;
	else
		x[0] += 48;
	if (x[1] > 9)
		x[1] += 87;
	else
		x[1] += 48;
	x[2] = '\0';
	return x;
}

int htoa (char *)
{
//16^7 = 268435456
//16^6 = 16777216
//16^5 = 1048576
//16^4 = 65536
//16^3 = 4096
//16^2 = 256
//16^1 = 16
//16^0 = 1


return 0;
}

std::string gin ()
{
	std::string in;
	char key = 0;
	while (read (0, &key, 1) != -1)
	{
		if (key == 0)
		{
			continue;
		}
		else if (key == 8 or key == 127)
		{
			if (in.size () > 0)
				in.pop_back ();
			write (1, "\r\x1b[2K", 5);
			write (1, in.c_str (), in.size ());
		}
		else if (key == 10 or key == 13 or key == '|')
		{
			in.push_back (key);
			break;
		}
		else
		{
			in.push_back (key);
			write (1, &key, 1);
		}
		key == 0;
	}
	return in;
}

void layer1 (char k)
{
	switch (s)
	{
		case -1:
		case 0:
		{
			break;
		}
	}
}

void layer2 (char k)
{
	switch (k)
	{
		case 'a':
		case 'c':
		case 'i':
		case 'd':
		case 'y':
		case 'v':
		case 'z':
		case 'm':
		case 'p':
		case 'x':
		case 'u':
		case 'r':
		case 'o':
		case 's':
		case 'n':
		case 'w':
		case 'q':
		case 'f':
		case '*':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case '\'':
		case '$':
		case '/':
		case '?':
		case '+':
		case '-':
		{
			break;
		}
		default:
		{

		}
	}
}

std::string getText ()
{
	std::string buffer;
	int word_counter, hex_counter = 0;
	std::stringstream sts;
	char key = 0, hex[2] = {48, 48}, t[1];
	while (1)
	{
		sts.str ("");
		sts
			//<< "0x"
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< word_counter
			<< '|'
			<< "\x1b[0m";
		write (1, sts.str ().c_str (), sts.str ().size ());
		sts.clear ();
		while (1)
		{
			key = 0;
			while (key == 0)
			{
				if (read (0, &key, 1) == -1)
				{
					exit (5);
				}
				if (key > 47 and key < 58)
				{
					hex[0] = key;
				}
				else if (key > 96 and key < 103)
				{
					hex[0] = key;
				}
				else if (key > 64 and key < 71)
				{
					key += 32;
					hex[0] = key;
				}
				else if (key == '.')
				{
					return buffer;
				}
				else
				{
					key = 0;
				}
			}
			key = 0;
			while (key == 0)
			{
				if (read (0, &key, 1) == -1)
				{
					exit (5);
				}
				if (key > 47 and key < 58)
				{
					hex[0] = key;
				}
				else if (key > 96 and key < 103)
				{
					hex[0] = key;
				}
				else if (key > 64 and key < 71)
				{
					key += 32;
					hex[0] = key;
				}
				else if (key == '.')
				{
					return buffer;
				}
				else
				{
					key = 0;
				}
			}
			t[0] = toC (hex);
			buffer.push_back (t[0]);
			hex_counter ++;
			if (hex_counter < 16)
			{
				write (1, "-", 1);
			}
			else
			{
				hex_counter = 0;
				word_counter ++;
				char p[1];
				write (1, "|", 1);
				for (int i = 0; i < 16; i ++)
				{
					p[0] = buffer.at(buffer.size () - 16 + i);
					if (p[0] > 31 and p[0] < 177)
						write (1, p, 1);
					else
						write (1, " ", 1);
				}
				write (1, "\n", 1);
				break;
			}
		}
	}
}

void printo ()
{
	std::stringstream s;
	s.str ("");
	if (buffer->b.size () > 0 or buffer->o < buffer->b.size ())
	{
		s
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< buffer->o
			<< '|';
		for (int i = 0; i < 16; i ++)
		{
			if (buffer->o + i < buffer->b.size ())
			{
				s << toH (buffer->b[buffer->o + i]);
			}
			else
			{
				s << "EOF";
				for (int j = 0; j < 16 - i; j++)
					s << "   ";
				break;
			}
			if (i < 16 - 1)
				s << '-';
		}
		s << '|';
		for (int i = 0; i < 16; i ++)
		{
			if (buffer->o + i < buffer->b.size ())
			{
				if (buffer->b[buffer->o + i] > 31 and buffer->b[buffer->o + i] < 177)
					s << buffer->b[buffer->o + i];
				else
					s << ' ';
			}
			else
			{
				s << '-';
			}
		}
		s << '.';
	}
	else
	{
		s << "!";
	}
	write (1, s.str ().c_str (), s.str ().size ());
}

void printo (int b)
{
	std::stringstream s;
	s.str ("");
	if (buffer->b.size () > 0 or b < buffer->b.size ())
	{
		s
			<< std::hex
			<< std::setw (8)
			<< std::right
			<< std::setfill ('0')
			<< b
			<< '|';
		for (int i = 0; i < 16; i ++)
		{
			if (b + i < buffer->b.size ())
			{
				s << toH (buffer->b[b + i]);
			}
			else
			{
				s << "EOF";
				for (int j = 0; j < 16 - i; j++)
					s << "   ";
				break;
			}
			if (i < 16 - 1)
				s << '-';
		}
		s << '|';
		for (int i = 0; i < 16; i ++)
		{
			if (b + i < buffer->b.size ())
			{
				if (buffer->b[b + i] > 31 and buffer->b[b + i] < 177)
					s << buffer->b[b + i];
				else
					s << ' ';
			}
			else
			{
				s << '-';
			}
		}
		s << '.';
	}
	else
	{
		s << "!";
	}
	write (1, s.str ().c_str (), s.str ().size ());
}
