# Why Bonobo?
Simplicity is beautiful. Websites are useful. I want to create useful website in the simplest way possible. Bonobo does this better than any other static site generator. It's made to be so simple a bonobo could use it. The code is meant to be so simple and readable that a caveman could easily understand the entire codebase. It's written in C for this reason. There's no magic behind the hood, there's nothing fancy. It does what it's supposed to in the simplest way possible.
**Features**:
* No need to write HTML. Use markdown for everything (html can be embedded if needed though)
* No need to write seperate config files. All metadata / configuration can be embedded in the markdown file or specified at runtime
* Simplicity. There is no "learning" Bonobo you install it and you run it. It runs in the most obvious way possible.

To look at examples of Bonobo files look at the test directory in this repo. Metadata (json format) is put at the top of the files to define things such as the description, title, language, and css stylesheet.
## For developers
Bonobo is designed to be as simple as possible thus dynamic memory allocation is avoided wherever possible. No variable length arrays at all and malloc() is only called for reading markdown files (so once in the whole program). Static char buffers are always preferred to dynamic memory allocations. For this reason Bonobo uses jsmn for parsing json and md4c for parsing markdown. This avoids a lot of possible side effects. Concurrency and parallelism is also avoided because it'd cause too many side effects to justify negligable speeds gains. Bonobo inherits a lot of speed already just by being written in C.

I hope Bonobo's codebase is a joy to for any developers or users that want to read through and understand it.
