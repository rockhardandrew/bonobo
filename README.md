# Why Bonobo?
This is as simple as a static site generator can get. The codebase is tiny and easy to modify to make your own. It's written in simple C so that it's effortless to understand it in it's entirety.

**Features**:
* No need to ever write HTML. Write in markdown (you can still embed html into markdown if necessary)
* RSS is supported
* Simplicity. There's zero learning curve there's no "learning" Bonobo. It's written to do everything in the most obvious way possible.

## Format
A bonobo page is split into metadata and content. Metadata is at the top of the file in json format and content is after in markdown format.
Some things that can be specified in the file metadata are the "title" (title of the page), "description" (description of page), "date" (MM/DD/YYYY format), "language" (what language the page is written in), and "css" (if this page uses a different stylesheet than the rest of the website),
Included in the "test" directory of this repo are some examples of bonobo pages
