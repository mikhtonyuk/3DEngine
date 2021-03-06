#include "SvgStylizable.h"

namespace Engine
{
	namespace UI
	{
		namespace SVG
		{
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			const std::string& SvgStylizable::CommonStyleName() const
			{
				return m_commonStyle;
			}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			SvgStyle& SvgStylizable::Style()
			{
				return m_style;
			}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			void SvgStylizable::SetStyle(const SvgStyle& style)
			{/*
				m_style.SetFillColor(style.FillColor());
				m_style.SetOpacity(style.Opacity());
				m_style.SetStrokeColor(style.StrokeColor());
				m_style.SetStrokeWidth(style.StrokeWidth());*/
				this->m_style = style;
			}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		}
	}
}
