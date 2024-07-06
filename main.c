
#include "base.h"
#include "os.h"

void OnInit(init_params *InitParams) {
    InitParams->WindowWidth = 720;
    InitParams->WindowHeight = 720;
}

void OnRender() {
}

/*
typedef struct kernel kernel;
typedef struct texture texture;

static kernel CCA;
static kernel Clear;

static texture ReadTexture;
static texture WriteTexture;

void OnInit(init_params *InitParams) {
    InitParams->WindowWidth = 1280;
    InitParams->WindowHeight = 720;

    ReadTexture = TextureCreate(512, 512);
    WriteTexture = TextureCreate(512, 512);
}

typedef struct {
    u32 Resolution;
    u32 Range;
    // etc.
} cca_constant_buffer;

cca_constant_buffer CCAParameters = {0};

void OnRender(texture OutputTexture) {

    BeginUI();
        Menu("Parameters");
            SliderU32("Resolution", &CCAParameters.Resolution, 32, 512, 32);
            SliderU32("Range", &CCAParameters.Range, 1, 8, 1);
        EndMenu();
    EndUI();

    KernelSetTexture(&CCA, 0, ReadTexture);
    KernelSetTexture(&CCA, 1, WriteTexture);
    KernelSetConstantBuffer(&CCA, &CCAParameters, sizeof(CCAParameters));
    KernelDispatch(&CCA, Buffer.Resolution / 32, Buffer.Resolution / 32, 1);

    BlitTexture(OutputTexture, WriteTexture);
}
*/