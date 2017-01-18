/** @file traceutils.h
 *  @brief contains the assembly helper functions for geting stack addresses
 *
 *
 *  @author todo
 */

#ifndef __TRACEUTILS_H
#define __TRACEUTILS_H


/** @brief just returns the frame pointer (ebp) of current function
 *
 *  In this case, we will only use it to start in the frame of traceback
 *
 *  @param no params
 *  @return the contents of ebp
 */
void* currframeptr();

/** @brief gets the return address (eip) of the caller function
 *
 *  Retrieves the address using the current ebp
 *
 *  @param ebp of the callee function
 *  @return eip of the caller function when callee returns
 */
void* callretaddr(void* ebp);

/** @brief gets the previous ebp based on the curr ebp
 *
 *  Retrieves the address using the current ebp
 *
 *  @param ebp of the callee function
 *  @return ebp of the caller function
 */
void* prevebp(void* ebp);


#endif /* __TRACEUTILS_H */