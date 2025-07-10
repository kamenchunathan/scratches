defmodule HeziWeb.IndexLive.Form do
  use HeziWeb, :live_view

  alias Hezi.Game
  alias Hezi.Game.Index

  @impl true
  def render(assigns) do
    ~H"""
    <Layouts.app flash={@flash}>
      <.header>
        {@page_title}
        <:subtitle>Use this form to manage index records in your database.</:subtitle>
      </.header>

      <.form for={@form} id="index-form" phx-change="validate" phx-submit="save">
        <.input field={@form[:room]} type="text" label="Room" />
        <footer>
          <.button phx-disable-with="Saving..." variant="primary">Save Index</.button>
          <.button navigate={return_path(@return_to, @index)}>Cancel</.button>
        </footer>
      </.form>
    </Layouts.app>
    """
  end

  @impl true
  def mount(params, _session, socket) do
    {:ok,
     socket
     |> assign(:return_to, return_to(params["return_to"]))
     |> apply_action(socket.assigns.live_action, params)}
  end

  defp return_to("show"), do: "show"
  defp return_to(_), do: "index"

  defp apply_action(socket, :edit, %{"id" => id}) do
    index = Game.get_index!(id)

    socket
    |> assign(:page_title, "Edit Index")
    |> assign(:index, index)
    |> assign(:form, to_form(Game.change_index(index)))
  end

  defp apply_action(socket, :new, _params) do
    index = %Index{}

    socket
    |> assign(:page_title, "New Index")
    |> assign(:index, index)
    |> assign(:form, to_form(Game.change_index(index)))
  end

  @impl true
  def handle_event("validate", %{"index" => index_params}, socket) do
    changeset = Game.change_index(socket.assigns.index, index_params)
    {:noreply, assign(socket, form: to_form(changeset, action: :validate))}
  end

  def handle_event("save", %{"index" => index_params}, socket) do
    save_index(socket, socket.assigns.live_action, index_params)
  end

  defp save_index(socket, :edit, index_params) do
    case Game.update_index(socket.assigns.index, index_params) do
      {:ok, index} ->
        {:noreply,
         socket
         |> put_flash(:info, "Index updated successfully")
         |> push_navigate(to: return_path(socket.assigns.return_to, index))}

      {:error, %Ecto.Changeset{} = changeset} ->
        {:noreply, assign(socket, form: to_form(changeset))}
    end
  end

  defp save_index(socket, :new, index_params) do
    case Game.create_index(index_params) do
      {:ok, index} ->
        {:noreply,
         socket
         |> put_flash(:info, "Index created successfully")
         |> push_navigate(to: return_path(socket.assigns.return_to, index))}

      {:error, %Ecto.Changeset{} = changeset} ->
        {:noreply, assign(socket, form: to_form(changeset))}
    end
  end

  defp return_path("index", _index), do: ~p"/game"
  defp return_path("show", index), do: ~p"/game/#{index}"
end
