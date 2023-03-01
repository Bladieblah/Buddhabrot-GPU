__constant sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_FILTER_NEAREST | CLK_ADDRESS_NONE;

/**
 * RNG stuff
 */

__constant ulong PCG_SHIFT = 6364136223846793005ULL;
__constant float PCG_MAX_1 = 4294967296.0;

#define PDF_SIDES 100
#define PDF_SIZE 201

#define UNIFORM_LOW 0.02425
#define UNIFORM_HIGH 0.97575

__constant float a[] = {
    -3.969683028665376e+01,
     2.209460984245205e+02,
    -2.759285104469687e+02,
     1.383577518672690e+02,
    -3.066479806614716e+01,
     2.506628277459239e+00
};

__constant float b[] = {
    -5.447609879822406e+01,
     1.615858368580409e+02,
    -1.556989798598866e+02,
     6.680131188771972e+01,
    -1.328068155288572e+01
};

__constant float c[] = {
    -7.784894002430293e-03,
    -3.223964580411365e-01,
    -2.400758277161838e+00,
    -2.549732539343734e+00,
     4.374664141464968e+00,
     2.938163982698783e+00
};

__constant float d[] = {
    7.784695709041462e-03,
    3.224671290700398e-01,
    2.445134137142996e+00,
    3.754408661907416e+00
};

inline float inverseNormalCdf(float u) {
	float q, r;

	if (u <= 0) {
		return -HUGE_VAL;
	}
	else if (u < UNIFORM_LOW) {
		q = sqrt(-2 * log(u));
		return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
			((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
	}
	else if (u <= UNIFORM_HIGH) {
        q = u - 0.5;
        r = q * q;
        
        return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
            (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1);
	}
	else if (u < 1) {
		q  = sqrt(-2 * log(1 - u));

		return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
			((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1);
	}
	else {
		return HUGE_VAL;
	}
}

inline ulong pcg32Random(global ulong *randomState, global ulong *randomIncrement, int x) {
    ulong oldstate = randomState[x];
    randomState[x] = oldstate * PCG_SHIFT + randomIncrement[x];
    uint xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint rot = oldstate >> 59u;
    uint pcg = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));

    return pcg;
}

__kernel void seedNoise(
    global ulong *randomState,
    global ulong *randomIncrement,
    global ulong *initState,
    global ulong *initSeq
) {
    const int x = get_global_id(0);

    randomState[x] = 0U;
    randomIncrement[x] = (initSeq[x] << 1u) | 1u;
    pcg32Random(randomState, randomIncrement, x);
    randomState[x] += initState[x];
    pcg32Random(randomState, randomIncrement, x);
}

inline float uniformRand(
    global ulong *randomState,
    global ulong *randomIncrement,
    int x
) {
    return (float)pcg32Random(randomState, randomIncrement, x) / PCG_MAX_1;
}

inline float gaussianRand(
    global ulong *randomState,
    global ulong *randomIncrement,
    int x
) {
    return inverseNormalCdf(uniformRand(randomState, randomIncrement, x));
}

/**
 * Complex math stuff
 */

 inline float2 cmul(float2 a, float2 b) {
    return (float2)( a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
 }

 inline float2 csquare(float2 z) {
    return (float2)( z.x * z.x - z.y * z.y, 2 * z.y * z.x);
 }

 inline float cnorm2(float2 z) {
    return z.x * z.x + z.y * z.y;
 }

 inline float cnorm(float2 z) {
    return sqrt(cnorm2(z));
 }

 inline float cdot(float2 a, float2 b) {
    return a.x * b.x + a.y * b.y;
 }

/**
 * Coordinate transformations
 */

 typedef struct ViewSettings {
    float scaleX, scaleY;
    float centerX, centerY;
    float theta, sinTheta, cosTheta;
    int sizeX, sizeY;
} ViewSettings;

inline float2 rotateCoords(float2 coords, ViewSettings view) {
    return (float2) {
        view.cosTheta * coords.x - view.sinTheta * coords.y,
        view.sinTheta * coords.x + view.cosTheta * coords.y
    };
}

// Transform fractal coordinates to screen coordinates, following openGL conventions
// The viewport spans [-1,1] in both dimensions
inline float2 fractalToScreen(float2 fractalCoord, ViewSettings view) {
    float2 tmp = rotateCoords((float2) {
        (fractalCoord.x - view.centerX),
        (fractalCoord.y - view.centerY)
    }, view);

    tmp.x = tmp.x / view.scaleX;
    tmp.y = tmp.y / view.scaleY;

    return tmp;
}

inline int2 screenToPixel(float2 screenCoord, ViewSettings view) {
    return (int2) {
        (1 + screenCoord.x) / 2 * view.sizeX,
        (1 + screenCoord.y) / 2 * view.sizeY
    };
}

 inline int2 fractalToPixel(float2 fractalCoord, ViewSettings view) {
    return screenToPixel(fractalToScreen(fractalCoord, view), view);
 }

/**
 * Fractal stuff
 */

typedef struct Particle {
    float2 pos;
    float2 offset, prevOffset;
    unsigned int iterCount;
    float score, prevScore;
} Particle;

__kernel void resetCount(global unsigned int *count, int size) {
    const int x = get_global_id(0);

    for (int i = 0; i < size; i++) {
        count[size * x + i] = 0;
    }
}

inline int matchThreshold(
    Particle particle,
    global unsigned int *threshold,
    int thresholdCount
) {
    for (int i = 0; i < thresholdCount; i++) {
        if (particle.iterCount <= threshold[i]) {
            return i;
        }
    }

    return -1;
}

inline void addPath(
    Particle *particle,
    global float2 *path,
    global unsigned int *count,
    global unsigned int *threshold,
    int thresholdCount,
    unsigned int pathStart,
    int thresholdIndex,
    ViewSettings view
) {
    unsigned int pixelCount = view.sizeX * view.sizeY;
    // float deltaScore = 1. / (float)particle->iterCount;
    
    for (unsigned int i = 0; i < particle->iterCount; i++) {
        int2 pixel = fractalToPixel(path[pathStart + i], view);

        if (! (pixel.x < 0 || pixel.x >= view.sizeX || pixel.y < 0 || pixel.y >= view.sizeY)) {
            atomic_inc(&count[thresholdIndex * pixelCount + view.sizeX * pixel.y + pixel.x]);
            particle->score += 1;
        }

        path[pathStart + i].y = -path[pathStart + i].y;
        pixel = fractalToPixel(path[pathStart + i], view);

        if (! (pixel.x < 0 || pixel.x >= view.sizeX || pixel.y < 0 || pixel.y >= view.sizeY)) {
            atomic_inc(&count[thresholdIndex * pixelCount + view.sizeX * pixel.y + pixel.x]);
            particle->score += 1;
        }
    }

    particle->score = pown(particle->score, 2);
}

constant float2 CENTER_1 = {-0.1225611668766536, 0.7448617666197446};
constant float RADIUS_1 = 0.095;

constant float2 CENTER_2 = {-1.3107026413368228, 0};
constant float2 CENTER_3 = {0.282271390766914, 0.5300606175785252};
constant float RADIUS_3 = 0.044;

constant float2 CENTER_4 = {-0.5043401754462431, 0.5627657614529813};
constant float RADIUS_4 = 0.037;
constant float2 CENTER_5 = {0.3795135880159236, 0.3349323055974974};
constant float RADIUS_5 = 0.0225;

inline bool isValid(float2 coord) {
    float c2 = cnorm2(coord);
    float a = coord.x;

    if (c2 >= 4) {
        return false;
    }
    
    // Main bulb
    if (256.0 * c2 * c2 - 96.0 * c2 + 32.0 * a < 3.0) {
        return false;
    }

    // Head
    if (16.0 * (c2 + 2.0 * a + 1.0) < 1.0) {
        return false;
    }

    coord.y = fabs(coord.y);

    // 2-step bulbs
    if (cnorm(coord - CENTER_1) < RADIUS_1) {
        return false;
    }

    // 3-step bulbs
    if (cnorm2(coord - CENTER_2) < RADIUS_3) {
        return false;
    }
    if (cnorm2(coord - CENTER_3) < RADIUS_3) {
        return false;
    }

    // 4-step bulbs
    if (cnorm2(coord - CENTER_4) < RADIUS_4) {
        return false;
    }
    if (cnorm2(coord - CENTER_5) < RADIUS_5) {
        return false;
    }

    return true;
}

inline void resetParticle(
    Particle *particle,
    global float2 *path,
    unsigned int pathStart,
    global ulong *randomState,
    global ulong *randomIncrement,
    int x
) {
    float2 newOffset;
    
    newOffset = (float2)(
        (4. * uniformRand(randomState, randomIncrement, x) - 2.5),
        (2.4 * uniformRand(randomState, randomIncrement, x) - 1.2)
    );

    // for (int i = 0; i < 50; i++) {
    //     newOffset = (float2)(
    //         (9. * uniformRand(randomState, randomIncrement, x) - 5.2),
    //         (6. * uniformRand(randomState, randomIncrement, x) - 3.)
    //     );

    //     if (isValid(newOffset)) {
    //         break;
    //     }
    // }

    particle->iterCount = 1;
    particle->pos = newOffset;
    particle->offset = newOffset;
    particle->prevOffset = newOffset;
    particle->score = 0;
    particle->prevScore = -1;

    path[pathStart] = newOffset;
}

inline float2 project(float2 v, float2 u1, float2 u2) {
    float u11 = cnorm2(u1);
    float u22 = cnorm2(u2);
    float u12 = cdot(u1, u2);

    float det = (u11 * u22 - pown(u12, 2));
    float idet = det > 0.001 ? 1. / det : 0;

    float a1 = cdot(v, u1);
    float a2 = cdot(v, u2);

    return (float2) {
        idet * (a1 * u22 - a2 * u12),
        idet * (a2 * u11 - a1 * u12)
    };
}

constant int MAX_CONVERGE_STEPS = 100;
constant float MAX_CONVERGE_STEP_SIZE =  0.02;

inline bool convergeParticle(
    Particle *particle,
    global float2 *path,
    unsigned int pathStart,
    global ulong *randomState,
    global ulong *randomIncrement,
    int x,
    ViewSettings view
) {
    float2 z = path[pathStart];
    float2 dzx = {1., 0.};
    float2 dzy = {0., 1.};

    float dist = 100;
    float2 dzxOpt = dzx;
    float2 dzyOpt = dzy;
    int iOpt = 0;

    int imax = min(MAX_CONVERGE_STEPS, (int)particle->iterCount);
    float2 target = (float2){-0.6, 0.6};

    for (int i = 1; i < imax; i++) {
        dzx = (float)2. * cmul(z, dzx) + (float2){1., 0.};
        dzy = (float)2. * cmul(z, dzy) + (float2){0., 1.};
        z = path[pathStart + i];
        
        float testDist = cnorm(target - z);

        if (testDist < dist) {
            dist = testDist;
            dzxOpt = dzx;
            dzyOpt = dzy;
            iOpt = i;
        }
    }

    particle->score = dist;

    if (dist < 0.01) {
        return true;
    }


    float2 diff = target - path[pathStart + iOpt];
    float2 step = project(diff, dzxOpt, dzyOpt);
    float stepSize = cnorm(step);

    if (stepSize > MAX_CONVERGE_STEP_SIZE) {
        step = step / stepSize * MAX_CONVERGE_STEP_SIZE;
    }

    float2 newOffset = particle->offset + (float)0.5 * step;

    particle->prevOffset = step;

    particle->pos = newOffset;
    particle->offset = newOffset;
    particle->iterCount = 1;

    path[pathStart] = newOffset;

    return dist < view.scaleY;
}

inline void mutateParticle(
    Particle *particle,
    global float2 *path,
    unsigned int pathStart,
    global ulong *randomState,
    global ulong *randomIncrement,
    int x,
    ViewSettings view
) {
    if (particle->score > particle->prevScore || particle->score / particle->prevScore > uniformRand(randomState, randomIncrement, x)) {
        particle->prevScore = particle->score;
        particle->prevOffset = particle->offset;
    }

    float2 newOffset = (float2)(
        particle->prevOffset.x + 0.01 * view.scaleY * clamp(gaussianRand(randomState, randomIncrement, x), -5., 5.),
        particle->prevOffset.y + 0.01 * view.scaleY * clamp(gaussianRand(randomState, randomIncrement, x), -5., 5.)
    );

    particle->pos = newOffset;
    particle->offset = newOffset;
    particle->iterCount = 1;
    particle->score = 0;

    path[pathStart] = newOffset;
}

__kernel void initParticles(
    global Particle *particles,
    global unsigned int *threshold,
    global float2 *path,
    global ulong *randomState,
    global ulong *randomIncrement,
    int thresholdCount
) {
    const int x = get_global_id(0);

    Particle tmp = particles[x];
    resetParticle(&tmp, path, x * threshold[thresholdCount - 1], randomState, randomIncrement, x);
    particles[x] = tmp;
}

__kernel void mandelStep(
    global Particle *particles,
    global unsigned int *count,
    global unsigned int *threshold,
    global float2 *path,
    global ulong *randomState,
    global ulong *randomIncrement,
    int thresholdCount,
    ViewSettings view
) {
    const int x = get_global_id(0);
    const unsigned int maxLength = threshold[thresholdCount - 1];
    const unsigned int pathIndex = x * maxLength;

    Particle tmp = particles[x];

    for (int i = 0; i < 1; i++) {
        tmp.pos = csquare(tmp.pos) + tmp.offset;
        path[pathIndex + tmp.iterCount] = tmp.pos;
        tmp.iterCount++;

        // tmp.pos = csquare(tmp.pos) + tmp.offset;
        // path[pathIndex + tmp.iterCount] = tmp.pos;
        // tmp.iterCount++;

        // tmp.pos = csquare(tmp.pos) + tmp.offset;
        // path[pathIndex + tmp.iterCount] = tmp.pos;
        // tmp.iterCount++;

        // tmp.pos = csquare(tmp.pos) + tmp.offset;
        // path[pathIndex + tmp.iterCount] = tmp.pos;
        // tmp.iterCount++;

        // tmp.pos = csquare(tmp.pos) + tmp.offset;
        // path[pathIndex + tmp.iterCount] = tmp.pos;
        // tmp.iterCount++;

        if (tmp.iterCount > 100 || cnorm2(tmp.pos) > 16) {
            convergeParticle(&tmp, path, pathIndex, randomState, randomIncrement, x, view);
        }

        // if (tmp.prevScore == -1 && tmp.iterCount > 100) {
        //     if (convergeParticle(&tmp, path, pathIndex, randomState, randomIncrement, x, view)) {
        //         tmp.prevScore = 0;
        //     }
        //     continue;
        // }

        // if (fabs(tmp.pos.x) > 4 || fabs(tmp.pos.y) > 4 || cnorm2(tmp.pos) > 16) {
        //     int thresholdIndex = matchThreshold(tmp, threshold, thresholdCount);
        //     addPath(&tmp, path, count, threshold, thresholdCount, pathIndex, thresholdIndex, view);
            
        //     if (tmp.prevScore < 0 && tmp.score > 0) {
        //         tmp.prevOffset = tmp.offset;
        //         tmp.prevScore = tmp.score;
        //     }

        //     if (tmp.prevScore > 5){
        //         mutateParticle(&tmp, path, pathIndex, randomState, randomIncrement, x, view);
        //     } else {
        //         convergeParticle(&tmp, path, pathIndex, randomState, randomIncrement, x, view);
        //     }
        // }

        // if (tmp.iterCount >= maxLength) {
        //     if (tmp.prevScore == -1) {
        //         resetParticle(&tmp, path, pathIndex, randomState, randomIncrement, x);
        //     } else {
        //         mutateParticle(&tmp, path, pathIndex, randomState, randomIncrement, x, view);
        //     }
        // }
    }

    particles[x] = tmp;
}

/**
 * Global operations, to be optimised later
 */

__kernel void findMax1(global unsigned int *count, global unsigned int *maxima, unsigned int size) {
    const int x = get_global_id(0);
    maxima[x] = 0;

    for (int i = x * size; i < (x + 1) * size; i++) {
        if (count[i] > maxima[x]) {
            maxima[x] = count[i];
        }
    }
}

__kernel void findMax2(global unsigned int *maxima, global unsigned int *maximum, unsigned int size) {
    const int x = get_global_id(0);
    maximum[x] = 0;

    for (int i = x * size; i < (x + 1) * size; i++) {
        if (maxima[i] > maximum[x]) {
            maximum[x] = maxima[i];
        }
    }
}

/**
 * Rendering
 */

__constant float COLOR_SCHEME[4][3] = {
    {0.1, 0.0, 0.2,},
    {0.0, 0.5, 0.6,},
    {0.8, 0.5, 0.0,},
    {0.1, 0.0, 0.2,},
};

__constant float IMAGE_MAX = 4294967295.0;

 __kernel void renderImage(
    global unsigned int *count,
    global unsigned int *maximum,
    global unsigned int *image,
    int thresholdCount
) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);

    const int W = get_global_size(0);
    const int H = get_global_size(1);

    const unsigned int pixelOffset = W * y + x;
    unsigned int pixelCount = W * H;
    const unsigned int imageOffset = 3 * pixelOffset;

    for (int j = 0; j < 3; j++) {
        image[imageOffset + j] = 0;

        for (int i = 0; i < thresholdCount; i++) {
            float countFraction = (float)count[i * pixelCount + pixelOffset] / (float)maximum[i];
            image[imageOffset + j] += (int)(COLOR_SCHEME[i][j] * sqrt(countFraction) * IMAGE_MAX);
        }

        if (image[imageOffset + j] >= 4294967295) {
           image[imageOffset + j] = 4294967295;
        }
    }
}
