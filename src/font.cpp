// Copyright (C) 2008-2011 by Philipp Muenzel. All rights reserved
// Released under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation, Inc.

#include "font.h"

using namespace PPL;

Font::Font(const std::string& fname, unsigned int height)
{
    textures = new GLuint[128];
    h = height;

    FT_Library library;
    if (FT_Init_FreeType( &library ))
        throw std::runtime_error("FT_Init_FreeType failed");

    FT_Face face;
    if (FT_New_Face( library, fname.c_str(), 0, &face ))
        throw std::runtime_error("FT_New_Face failed (there is probably a problem with your font file)");

    FT_Set_Char_Size( face, height << 6, height << 6, 96, 96);

    list_base=glGenLists(128);
    XPLMGenerateTextureNumbers((int*)textures, 128);

    for(unsigned char i = 0 ; i < 128 ; i++)
        make_dlist(face,i,list_base,textures);

    FT_Done_Face(face);
    FT_Done_FreeType(library);
}

Font::~Font()
{
    glDeleteLists(list_base,128);
    glDeleteTextures(128,textures);
    delete [] textures;
}

void Font::make_dlist ( FT_Face face, char ch, GLuint list_base, GLuint * tex_base )
{
    if(FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT ))
        throw std::runtime_error("FT_Load_Glyph failed");

    FT_Glyph glyph;
    if(FT_Get_Glyph( face->glyph, &glyph ))
        throw std::runtime_error("FT_Get_Glyph failed");

    FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );
    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;

    FT_Bitmap& bitmap=bitmap_glyph->bitmap;

    int width = next_p2( bitmap.width );
    int height = next_p2( bitmap.rows );

    GLubyte* expanded_data = new GLubyte[ 2 * width * height];

    for(int j=0; j <height;j++) {
        for(int j=0; j < height;j++) for(int i=0; i < width; i++) {
            expanded_data[2*(i+j*width)] = 255;
            expanded_data[2*(i+j*width)+1] =
                    (i>=bitmap.width || j>=bitmap.rows) ?
                        0 : bitmap.buffer[i + bitmap.width*j];
        }
    }

    XPLMBindTexture2d(tex_base[(int)ch], 0);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                  0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, expanded_data );

    delete [] expanded_data;

    glNewList(list_base+ch,GL_COMPILE);
    XPLMBindTexture2d(tex_base[(int)ch], 0);

    glPushMatrix();

    glTranslatef(bitmap_glyph->left,0,0);
    glTranslatef(0,bitmap_glyph->top-bitmap.rows,0);

    float x = bitmap.width / static_cast<float>(width);
    float y = bitmap.rows  / static_cast<float>(height);

    glBegin(GL_QUADS);
    glTexCoord2d(0,y); glVertex2f(0,0);
    glTexCoord2d(0,0); glVertex2f(0,bitmap.rows);
    glTexCoord2d(x,0); glVertex2f(bitmap.width,bitmap.rows);
    glTexCoord2d(x,y); glVertex2f(bitmap.width,0);
    glEnd();
    glPopMatrix();

    glTranslatef(face->glyph->advance.x >> 6 ,0,0);

    glEndList();
    FT_Done_Glyph(glyph);
}

int Font::next_p2 (int a)
{
    int rval=2;
    while(rval<a)
        rval<<=1;
    return rval;
}



void PPL::glPrint(const Font& ft_font, float x, float y, const std::string& text)
{
    GLuint font=ft_font.list_base;

    glPushAttrib(GL_LIST_BIT | GL_CURRENT_BIT  | GL_ENABLE_BIT | GL_TRANSFORM_BIT);
//    float color[4];
//    glGetFloatv(GL_CURRENT_COLOR,color);

//    XPLMSetGraphicsState(0,1,0,0,1,0,0);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glListBase(font);

    float modelview_matrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview_matrix);

    glPushMatrix();
    glLoadIdentity();
//    glColor4fv(color);
    glTranslatef(x,y,0);
    glMultMatrixf(modelview_matrix);
    glCallLists(text.size(), GL_UNSIGNED_BYTE, text.c_str());
    glPopMatrix();

    glPopAttrib();
}

