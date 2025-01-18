class GLShaderData:
    def __init__(self):
        self.defines = []
        self.uniform_enums = []
        self.uniform_strs = []
        self.tex_unit_names = []
        self.vertex_code = []
        self.fragment_code = []
        self.geo_code = []
        self.compute_code = []


def parse_gl_file(shader_data, filename):
    sf = open(filename, "r")    

    line = sf.readline()

    while line:
        #handle defines
        if line.find("#ifdef") != -1:
            define_str = line.replace("#ifdef", "").strip()
            
            if define_str not in shader_data.defines:
                shader_data.defines.append(define_str)
        if line.find("#ifndef") != -1:
            define_str = line.replace("#ifndef", "").strip()
            
            if define_str not in shader_data.defines:
                shader_data.defines.append(define_str)
        #handle uniforms
        if line.find("uniform") != -1 and line.find("u_") != -1 and line.find("sampler") == -1:
            
            uni_str = line.strip("uniform")
            uni_str = uni_str.replace(";", "")
            uni_lines = uni_str.split()

            for each in uni_lines:
                each = each.strip()
                if each.find("u_") != -1:
                    uni_name = each
                    each = each.removeprefix("u_")
                    
                    enum_name = each.capitalize().strip()

                    #array
                    if each.find("[") != -1:
                        uni_name = each[: each.find("[")]
                        enum_name = uni_name.removeprefix("u_").capitalize().strip() 
                        uni_name = "u_" + uni_name

                    if uni_name not in shader_data.uniform_strs:
                        shader_data.uniform_strs.append(uni_name)

                    if enum_name not in shader_data.uniform_enums:
                        shader_data.uniform_enums.append(enum_name)

        #handle tex uniforms
        elif line.find("uniform") != -1 and line.find("sampler") != -1:
            line = line.strip("uniform")
            line = line.replace(";", "")
            uni_lines = line.split(",")

            for each in uni_lines:
                each.strip()
                each = each[each.rfind(" ") + 1 :]

                #array
                if each.find("[") != -1:
                    each = each[: each.find("[")]
                     
                if each not in shader_data.tex_unit_names:
                    shader_data.tex_unit_names.append(each)

        line = sf.readline()
    
    sf.seek(0)

    line = sf.readline()

    if filename.find(".vert") != -1:
        while line:
            shader_data.vertex_code.append(line)

            line = sf.readline()
    
    elif filename.find(".frag") != -1:
        while line:
            shader_data.fragment_code.append(line)

            line = sf.readline()
    elif filename.find(".geo") != -1:
        while line:
            shader_data.geo_code.append(line)

            line = sf.readline()
    elif filename.find(".comp") != -1:
        while line:
            shader_data.compute_code.append(line)

            line = sf.readline()


    sf.close()

    return shader_data

def write_gl_header(file, filename):
    shader_data = GLShaderData()
    for each in filename:
        shader_data = parse_gl_file(shader_data, each)

    if filename[0].find("/") != -1:
        filename[0] = filename[0][filename[0].find("/") + 1 :]
    prefix = filename[0][: filename[0].find(".")].strip().upper()
    name = prefix
    prefix = prefix + "_"

    print("Writing " + name + " Shader \n")

    file.write("// " + name + " SHADER SECTION \n")

    #define enums
    if shader_data.defines:
        file.write("typedef enum \n")
        file.write("{\n")

        for each in shader_data.defines:
            file.write("    " + prefix + "DEFINE_" + each + ",\n")
        
        file.write("    " + prefix + "DEFINE_MAX\n")
        file.write("}")
        file.write(prefix + "SHADER_DEFINES; \n")
        file.write("\n")
    
    #uniform enums
    if shader_data.uniform_enums:
        file.write("typedef enum \n")
        file.write("{\n")

        for each in shader_data.uniform_enums:
            file.write("    " + prefix + "UNIFORM_" + each.upper() + ",\n")
        
        file.write("    " + prefix + "UNIFORM_MAX\n")
        file.write("}")
        file.write(prefix + "SHADER_UNIFORMS; \n")
        file.write("\n")

    #define names
    if shader_data.defines:
        file.write("static const char* " + prefix + "DEFINES_STR[] = \n")
        file.write("{\n")

        for each in shader_data.defines:
            file.write("    \"" + each + "\", \n")

        file.write("};\n")
        
    #uniform names
    if shader_data.uniform_strs:
        file.write("static const char* " + prefix + "UNIFORMS_STR[] = \n")
        file.write("{\n")

        for each in shader_data.uniform_strs:
            file.write("    \"" + each + "\", \n")

        file.write("};\n")

    #texture names
    if shader_data.tex_unit_names:
        file.write("static const char* " + prefix + "TEXTURES_STR[] = \n")
        file.write("{\n")

        for each in shader_data.tex_unit_names:
            file.write("    \"" + each.strip() + "\", \n")

        file.write("};\n")
        file.write("\n")

    #vertex code
    """
    if shader_data.vertex_code:
        file.write("static const char " + prefix + "VERTEX_CODE[] = \n")
        file.write("{\n")

        for each in shader_data.vertex_code:
            for l in each:
                file.write(str(ord(l)) + ",")
            file.write(str(ord("\n")) + ",")

        file.write("\n};\n")
        file.write("\n")
    #fragment code
    if shader_data.fragment_code:
        file.write("static const char " + prefix + "FRAGMENT_CODE[] = \n")
        file.write("{\n")

        for each in shader_data.fragment_code:
            for l in each:
                file.write(str(ord(l)) + ",")
            file.write(str(ord("\n")) + ",")

        file.write("\n};\n")
        file.write("\n")
    if shader_data.geo_code:
        file.write("static const char " + prefix + "GOEMETRY_CODE[] = \n")
        file.write("{\n")

        for each in shader_data.geo_code:
            for l in each:
                file.write(str(ord(l)) + ",")
            file.write(str(ord("\n")) + ",")

        file.write("\n};\n")
        file.write("\n")
    if shader_data.compute_code:
        file.write("static const char " + prefix + "COMPUTE_CODE[] = \n")
        file.write("{\n")

        for each in shader_data.compute_code:
            for l in each:
                file.write(str(ord(l)) + ",")
            file.write(str(ord("\n")) + ",")

        file.write("\n};\n")
        file.write("\n")
    """


def write_dir_gl_headers():
    filename = "shader_info.h"

    file = open(filename, "w")
    
    file.write("#ifndef SHADER_INFO_H \n")
    file.write("#define SHADER_INFO_H \n")
    file.write("#pragma once \n")


    file.write("/*\n ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n")
    file.write("SHADER INFORMATION. Shader enums, uniforms, etc... \nGenerated by a python script shader_info_generator.py \n")
    file.write("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n */ \n")

    write_gl_header(file, ["scene_3d.vert", "scene_3d_deferred.frag", "scene_3d_forward.frag"])
    write_gl_header(file, ["screen/post_process.frag"])
    write_gl_header(file, ["screen/deferred_scene.frag"])
    write_gl_header(file, ["screen/ssao.comp"])
    write_gl_header(file, ["screen/dof.comp"])
    write_gl_header(file, ["screen/brdf.frag"])
    write_gl_header(file, ["screen/godray.comp"])
    write_gl_header(file, ["screen/box_blur.comp"])
    write_gl_header(file, ["screen/bloom.frag"])
    write_gl_header(file, ["screen/screen_shader.vert", "screen/screen_shader.frag"])
    write_gl_header(file, ["screen/debug_screen.frag"])
    write_gl_header(file, ["lc_world/lc_world.vert", "lc_world/lc_world.frag"])
    write_gl_header(file, ["lc_world/lc_occlusion_boxes.vert", "lc_world/lc_occlusion_boxes.frag"])
    write_gl_header(file, ["lc_world/lc_water.vert", "lc_world/lc_water.frag"])
    write_gl_header(file, ["lc_world/process_chunks.comp"])
    write_gl_header(file, ["cubemap/cubemap.vert", "cubemap/cubemap.frag"])

    file.write("\n#endif")

    file.close()
    
    #print("Shader info data writen to " + filename + " \n")
   # print("Press enter to continue..\n")
    #input()

write_dir_gl_headers()



