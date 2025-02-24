## Bookipedia

### Description
A console program that stores books in a Postgres database.\
The program uses the "Repository" pattern.

#### Input and output data:
The standard input receives commands, one per line. The commands have the following format:
```Bash
Command <command arguments>
```
The program terminates after stdin runs out of data.

#### Command AddAuthor
The `AddAuthor` <name> command adds a book author with the specified name to the database, and the author id is generated randomly. The name consists of one or more words separated by spaces. Spaces at the beginning and end of the name are removed.\
Examples:
```Bash
AddAuthor Joanne Rowling
AddAuthor Antoine de Saint-Exupery
```

#### Command DeleteAuthor
The `DeleteAuthor` command deletes the selected author, all of their books, and the tags associated with those books. Books by other authors and their tags are not affected.
```Bash
DeleteAuthor
1 Jack London
2 Joanne Rowling
Enter author # or empty line to cancel
1
ShowAuthors
1 Joanne Rowling
```
You can specify the name of the author to be deleted directly in the `DeleteAuthor` command:
```Bash
ShowAuthors
1 Jack London
2 Joanne Rowling
DeleteAuthor Jack London
ShowAuthors
1 Joanne Rowling
```

#### Command EditAuthor
The `EditAuthor` command is used to edit the selected author. You can specify the author's name in the command or select from the list:
```Bash
EditAuthor
1 Jack London
2 Joanne Rowling
Enter author # or empty line to cancel
2
Enter new name:
J. K. Rowling
EditAuthor Jack London
Enter new name:
John Griffith Chaney
ShowAuthors
1 J. K. Rowling
2 John Griffith Chaney
```

#### Command ShowAuthors
The `ShowAuthors` command has no arguments. It outputs a numbered list of authors, sorted alphabetically.\
Example:
```Bash
AddAuthor Joanne Rowling
AddAuthor Jack London
ShowAuthors
1. Jack London
2. Joanne Rowling
```
If the authors list is empty, the `ShowAuthors` command does not output anything.\

#### Command AddBook
The `AddBook <pub_year> <title>` command adds a book with the specified title and year of publication to the database. When executing the command, The program will prompt the user to enter the author's name directly or select the author from the list provided. The list sorted alphabetically. If the user entered the author's name manually and there is no such author among the authors, the program will prompt the user to add the author automatically.\

If an empty line is entered, adding the book is canceled. If a valid author number is entered, the book is added to the books table.\
Examples:
```Bash
AddBook 1906 White Fang
Enter author name or empty line to select from list:
Jack London
No author found. Do you want to add Jack London (y/n)?
y
```

If the user agrees to add an author, then a new author is added to the authors table and the book data entry continues. The program will ask to enter tags that relate to the book, separating them with commas.
```Bash
Enter tags (comma separated):
adventure, dog,   gold   rush  ,  dog,,dogs
```
A book may have zero tags.

#### Command ShowAuthorBooks
The `ShowAuthorBooks` command has no parameters. In response to this command, a numbered list of author names, sorted alphabetically, is printed to stdout, and the program promtes you to enter author number, as in the 'AddAuthor' command. After selecting an author, a numbered list of books with their years of publication, sorted by year of publication, should be printed to stdout. If one or more books were published in the same year, they are printed in ascending order of their title.\
Example:
```Bash
ShowAuthorBooks
Select author:
1 Boris Akunin
2 Jack London
3 Joanne Rowling
4 Herman Melville
Enter author # or empty line to cancel
1
1 Azazelle, 1998
2 Murder on the Leviathan, 1998
3 The Turkish Gambit, 1998
4 She Lover of Death, 2001
```
If the entered author has no books, or an empty string is entered instead of the author's serial number, the command does not output anything.

#### Command ShowBooks
The `ShowBooks` command has no parameters. It outputs to stdout a numbered list of books. The book information includes the title, year of publication, author name and publication year.\
The books are sorted by author name then by publication year.
Output format:
```Bash
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906
```
If there are no books in the database, nothing is output to stdout.

#### Command ShowBook
The `ShowBook` ​​command displays detailed information about a book: author, title, year of publication, tags, if any. You can specify the book title in the command itself, or select it from the list.\
If there are several books with the entered title, the program offers to select it from the list.\
If the book has tags, the text “Tags: “ is displayed, followed by the tags in alphabetical order, separated by a comma and a space. If there are no tags, the line Tags: is not displayed.\
Example:
```Bash
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906
ShowBook White Fang
Title: White Fang
Author: Jack London
Publication year: 1906
Tags: adventure, dog, gold rush
ShowBook The Cloud Atlas
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
Enter the book # or empty line to cancel:
2
Title: The Cloud Atlas
Author: Liam Callanan
Publication year: 2004
ShowBook
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906
Enter the book # or empty line to cancel:
3
Title: White Fang
Author: Jack London
Publication year: 1906
Tags: adventure, dog, gold rush
```
If there is no book with the entered title, nothing will be displayed.

#### Command DeleteBook
The `DeleteBook` command allows you to delete a book by specifying its title directly or by selecting it from a list. If there are several books with the entered title, or the user has not entered its title, the program will display the existing books with this title and ask which book should be deleted.\
Example:
```Bash
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906
DeleteBook The Cloud Atlas
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
Enter the book # or empty line to cancel:
2
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 White Fang by Jack London, 1906
DeleteBook
1 The Cloud Atlas by David Mitchell, 2004
2 White Fang by Jack London, 1906
Enter the book # or empty line to cancel:
1
```

#### Command EditBook
The `EditBook` command allows you to edit a book. You can specify the book title directly or select it from a list. If there are several books with the entered title, the program will ask the user to select the desired book from the list.\
Example:
```Bash
ShowBooks
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fan by Jack London, 1906
EditBook White Fan
Enter new title or empty line to use the current one (White Fan):
White Fang
Enter publication year or empty line to use the current one (1906):

Enter tags (current tags: adventure, cat, gold rush):
adventure, gold rush, dog
ShowBook White Fang
Title: White Fang
Author: Jack London
Publication year: 1096
Tags: adventure, dog, gold rush
EditBook
1 The Cloud Atlas by David Mitchell, 2004
2 The Cloud Atlas by Liam Callanan, 2004
3 White Fang by Jack London, 1906
Enter the book # or empty line to cancel:
3
Enter new title or empty line to use the current one (White Fang):

Enter publication year or empty line to use the current one (1906):

Enter tags (current tags: adventure, dog, gold rush):
adventure, gold rush, dog, wolf
```

### Build

The program was tested on Ubuntu 22.04.\
You must have gcc 11.3 or later, python 3 installed.

Install the required packages:

```Bash
sudo apt update && apt install -y python3-pip cmake && pip3 install conan==1.*
```
Install Conan Package Manager:

```Bash
mkdir /build && cd /build && conan install .. -s compiler.libcxx=libstdc++11
```

Build the program:

```Bash
cd /build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
```

### Run

```Bash
cd /build
bookipedia
```
