## Bookipedia

### Description
A console program that stores books in a Postgres database.\
When running, the program read the database URL from the `BOOKYPEDIA_DB_URL` environment variable.\
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
The `AddBook <pub_year> <title>` command adds a book with the specified title and year of publication to the database. When executing the command, the program outputs the Select author: line, followed by a numbered list of authors sorted alphabetically. At the end, the program outputs the `Enter author # or empty line to cancel` line and read the author's serial number from stdin.\

If an empty line is entered, adding the book is canceled. If a valid author number is entered, the book is added to the books table.\
Examples:
```Bash
AddBook 1998 Harry Potter and the Chamber of Secrets
Select author:
1 Jack London
2 Joanne Rowling
Enter author # or empty line to cancel
2
AddBook 1851 Moby-Dick
Select author:
1 Jack Londong
2 Joanne Rowling
Enter author # or empty line to cancel

AddAuthor Herman Melville
AddBook 1851 Moby-Dick
Select author:
1 Jack Londong
2 Joanne Rowling
3 Herman Melville
Enter author # or empty line to cancel
3
```

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
The ShowBooks command has no parameters. It outputs to stdout a numbered list of books, sorted by title. The book information includes the title and year of publication.\
Example:
```Bash
ShowBooks
1 Fight Club, 1996
2 Harry Potter and the Chamber of Secrets, 1998
3 Romeo and Juliet, 1597
```
If there are no books in the database, nothing is output to stdout.
