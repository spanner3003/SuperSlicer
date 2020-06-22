#include "CalibrationAbstractDialog.hpp"
#include "I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "GUI.hpp"
#include "GUI_ObjectList.hpp"
#include "Tab.hpp"
#include <wx/scrolwin.h>
#include <wx/display.h>
#include <wx/file.h>

#if ENABLE_SCROLLABLE
static wxSize get_screen_size(wxWindow* window)
{
    const auto idx = wxDisplay::GetFromWindow(window);
    wxDisplay display(idx != wxNOT_FOUND ? idx : 0u);
    return display.GetClientArea().GetSize();
}
#endif // ENABLE_SCROLLABLE

namespace Slic3r {
namespace GUI {

    CalibrationAbstractDialog::CalibrationAbstractDialog(GUI_App* app, MainFrame* mainframe, std::string name)
        : DPIDialog(NULL, wxID_ANY, wxString(SLIC3R_APP_NAME) + " - " + _(L(name)),
#if ENABLE_SCROLLABLE
            wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
#else
        wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
#endif // ENABLE_SCROLLABLE
    {
        this->gui_app = app;
        this->main_frame = mainframe;
        SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

        // fonts
        const wxFont& font = wxGetApp().normal_font();
        const wxFont& bold_font = wxGetApp().bold_font();
        SetFont(font);

    }

void CalibrationAbstractDialog::create(std::string html_path, wxSize dialog_size){

    auto main_sizer = new wxBoxSizer(wxVERTICAL);

    //html
    html_viewer = new wxHtmlWindow(this, wxID_ANY,
        wxDefaultPosition, dialog_size, wxHW_SCROLLBAR_AUTO);
    html_viewer->LoadPage(Slic3r::resources_dir()+ html_path);
    main_sizer->Add(html_viewer, 1, wxEXPAND | wxALL, 5);

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    create_buttons(buttons);

    wxButton* close = new wxButton(this, wxID_CLOSE, _(L("Close")));
    close->Bind(wxEVT_BUTTON, &CalibrationAbstractDialog::close_me, this);
    buttons->AddButton(close);
    close->SetDefault();
    close->SetFocus();
    SetAffirmativeId(wxID_CLOSE);
    buttons->Realize();
    main_sizer->Add(buttons, 0, wxEXPAND | wxALL, 5);

    SetSizer(main_sizer);
    main_sizer->SetSizeHints(this);
}

void CalibrationAbstractDialog::close_me(wxCommandEvent& event_args) {
    this->gui_app->change_calibration_dialog(this, nullptr);
    this->Destroy();
}

ModelObject* CalibrationAbstractDialog::add_part(ModelObject* model_object, std::string input_file, Vec3d move, Vec3d scale) {
    Model model;
    try {
        model = Model::read_from_file(input_file);
    }
    catch (std::exception & e) {
        auto msg = _(L("Error!")) + " " + input_file + " : " + e.what() + ".";
        show_error(this, msg);
        exit(1);
    }

    for (ModelObject* object : model.objects) {
        Vec3d delta = Vec3d::Zero();
        if (model_object->origin_translation != Vec3d::Zero())
        {
            object->center_around_origin();
            delta = model_object->origin_translation - object->origin_translation;
        }
        for (ModelVolume* volume : object->volumes) {
            volume->translate(delta + move);
            if (scale != Vec3d{ 1,1,1 }) {
                volume->scale(scale);
            }
            ModelVolume* new_volume = model_object->add_volume(*volume);
            new_volume->set_type(ModelVolumeType::MODEL_PART);
            new_volume->name = boost::filesystem::path(input_file).filename().string();

            //volumes_info.push_back(std::make_pair(from_u8(new_volume->name), new_volume->get_mesh_errors_count() > 0));

            // set a default extruder value, since user can't add it manually
            new_volume->config.set_key_value("extruder", new ConfigOptionInt(0));

            //move to bed
            /* const TriangleMesh& hull = new_volume->get_convex_hull();
            float min_z = std::numeric_limits<float>::max();
            for (const stl_facet& facet : hull.stl.facet_start) {
                for (int i = 0; i < 3; ++i)
                    min_z = std::min(min_z, Vec3f::UnitZ().dot(facet.vertex[i]));
            }
            volume->translate(Vec3d(0,0,-min_z));*/
        }
    }
    assert(model.objects.size() == 1);
    return model.objects.empty()?nullptr: model.objects[0];
}

void CalibrationAbstractDialog::on_dpi_changed(const wxRect& suggested_rect)
{
    msw_buttons_rescale(this, em_unit(), { wxID_OK });

    Layout();
    Fit();
    Refresh();
}

wxPanel* CalibrationAbstractDialog::create_header(wxWindow* parent, const wxFont& bold_font)
{
    wxPanel* panel = new wxPanel(parent);
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

    wxFont header_font = bold_font;
#ifdef __WXOSX__
    header_font.SetPointSize(14);
#else
    header_font.SetPointSize(bold_font.GetPointSize() + 2);
#endif // __WXOSX__

    sizer->AddStretchSpacer();

    // text
    wxStaticText* text = new wxStaticText(panel, wxID_ANY, _(L("Keyboard shortcuts")));
    text->SetFont(header_font);
    sizer->Add(text, 0, wxALIGN_CENTER_VERTICAL);

    sizer->AddStretchSpacer();

    panel->SetSizer(sizer);
    return panel;
}

} // namespace GUI
} // namespace Slic3r
