/**
 * @mainpage
 *
 * - @ref page1
 * - @ref page2
 *
 * @page page1 Notes on the development
 *
 * @section sec1 Coding style
 *
 * @section sec2 Advices on modules design
 *
 * Common issues on composition of xwlib modules
 * and best design practices are discussed
 * in this section.
 * These are:
 *  - operating system interface wrappers
 *  - C strings usage
 *  - memory usage
 *
 * @subsection AVDICE_OS_API Operating system interface wrappers
 *
 * For debugging and runtime profiling purposes mostly all
 * operating system API calls should be wrapped with
 * internal commands. Currently most of used system calls
 * are wrapped in xwlib, @footnote{prototypes and macro definitions
 * are in the x_lib.h}.
 *
 * @subsection AVDICE_C_STRING C strings usage
 *
 * The best way to use C strings explicitly in xwlib are not to
 * use C strings explicitly, unless when this is only one possible
 * solution. Instead you should use special string type.
 * In case when using a const C strings in a source code surround
 * them with X_STRING() macro.
 * @code
 * // function accepts special string type
 * void foo (x_string_t *xstr);
 *
 * void bar(void)
 * {
 *      foo(X_STRING("Hello!\n"));
 * }
 * @endcode
 *
 * @subsection AVDICE_MEM Memory usage
 *
 * This time only one advice on memory usage:
 * Use x_malloc() and x_free() predefined functions to allocate
 * memory from inside a module.
 *
 *
 * @page page2 Xmppbot SDK Tutorial
 * SDK Overview.
 * @section sec Components structure
 * This is dependency graph of Xmppbot SDK components.
 * @image html deps.dot.png
 * @image latex deps.dot.eps "SDK components" width=10cm
 *
 */

