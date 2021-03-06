// Copyright 2017-2018 the openage authors. See copying.md for legal info.

#include "renderer.h"

#include "../../log/log.h"
#include "../../error/error.h"
#include "texture.h"
#include "shader_program.h"
#include "uniform_input.h"
#include "geometry.h"


namespace openage {
namespace renderer {
namespace opengl {

GlRenderer::GlRenderer(GlContext *ctx)
	: gl_context(ctx)
{
	log::log(MSG(info) << "Created OpenGL renderer");
}

std::unique_ptr<Texture2d> GlRenderer::add_texture(const resources::Texture2dData& data) {
	return std::make_unique<GlTexture2d>(data);
}

std::unique_ptr<Texture2d> GlRenderer::add_texture(const resources::Texture2dInfo& info) {
	return std::make_unique<GlTexture2d>(info);
}

std::unique_ptr<ShaderProgram> GlRenderer::add_shader(std::vector<resources::ShaderSource> const& srcs) {
	return std::make_unique<GlShaderProgram>(srcs, this->gl_context->get_capabilities());
}

std::unique_ptr<Geometry> GlRenderer::add_mesh_geometry(resources::MeshData const& mesh) {
	return std::make_unique<GlGeometry>(mesh);
}

std::unique_ptr<Geometry> GlRenderer::add_bufferless_quad() {
	return std::make_unique<GlGeometry>();
}

std::unique_ptr<RenderTarget> GlRenderer::create_texture_target(std::vector<Texture2d*> textures) {
	std::vector<const GlTexture2d*> gl_textures;
	gl_textures.reserve(textures.size());
	for (auto tex : textures) {
		gl_textures.push_back(static_cast<const GlTexture2d*>(tex));
	}

	return std::make_unique<GlRenderTarget>(gl_textures);
}

RenderTarget const* GlRenderer::get_display_target() {
	return &this->display;
}

resources::Texture2dData GlRenderer::display_into_data() {
	GLint params[4];
	glGetIntegerv(GL_VIEWPORT, params);

	GLint width = params[2];
	GLint height = params[3];

	resources::Texture2dInfo tex_info(width, height, resources::pixel_format::rgba8, 4);
	std::vector<uint8_t> data(tex_info.get_data_size());

	static_cast<GlRenderTarget const*>(this->get_display_target())->bind_read();
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadnPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tex_info.get_data_size(), data.data());

	resources::Texture2dData img(std::move(tex_info), std::move(data));
	return img.flip_y();
}

void GlRenderer::render(RenderPass const& pass) {
	auto gl_target = dynamic_cast<const GlRenderTarget*>(pass.target);
	gl_target->bind_write();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (auto obj : pass.renderables) {
		if (obj.alpha_blending) {
			glEnable(GL_BLEND);
		}
		else {
			glDisable(GL_BLEND);
		}

		if (obj.depth_test) {
			glEnable(GL_DEPTH_TEST);
		}
		else {
			glDisable(GL_DEPTH_TEST);
		}

		auto in = dynamic_cast<GlUniformInput const*>(obj.unif_in);
		auto geom = dynamic_cast<GlGeometry const*>(obj.geometry);
		in->program->execute_with(in, geom);
	}
}

}}} // openage::renderer::opengl
