#ifndef __DEPTH_BUFFER_MACRO__
#define __DEPTH_BUFFER_MACRO__


#define REVERSE_Z_ON
#define INFINITE_FAR_PLANE 

float depthToLinear(float viewZ, float nearPlane, float farPlane)
{
#ifdef REVERSE_Z_ON

    #ifdef INFINITE_FAR_PLANE
        return nearPlane / viewZ;
    #else
        return (farPlane * nearPlane) / (viewZ * (nearPlane - farPlane) + farPlane);
    #endif

#else

    #ifdef INFINITE_FAR_PLANE
        return 1.0 - (nearPlane / viewZ);
    #else
        return (viewZ * (farPlane - nearPlane) - nearPlane * farPlane) / (viewZ * (farPlane - nearPlane));
    #endif

#endif
}



#endif