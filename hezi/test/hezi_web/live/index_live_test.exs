defmodule HeziWeb.IndexLiveTest do
  use HeziWeb.ConnCase

  import Phoenix.LiveViewTest
  import Hezi.GameFixtures

  @create_attrs %{room: "some room"}
  @update_attrs %{room: "some updated room"}
  @invalid_attrs %{room: nil}
  defp create_index(_) do
    index = index_fixture()

    %{index: index}
  end

  describe "Index" do
    setup [:create_index]

    test "lists all game", %{conn: conn, index: index} do
      {:ok, _index_live, html} = live(conn, ~p"/game")

      assert html =~ "Listing Game"
      assert html =~ index.room
    end

    test "saves new index", %{conn: conn} do
      {:ok, index_live, _html} = live(conn, ~p"/game")

      assert {:ok, form_live, _} =
               index_live
               |> element("a", "New Index")
               |> render_click()
               |> follow_redirect(conn, ~p"/game/new")

      assert render(form_live) =~ "New Index"

      assert form_live
             |> form("#index-form", index: @invalid_attrs)
             |> render_change() =~ "can&#39;t be blank"

      assert {:ok, index_live, _html} =
               form_live
               |> form("#index-form", index: @create_attrs)
               |> render_submit()
               |> follow_redirect(conn, ~p"/game")

      html = render(index_live)
      assert html =~ "Index created successfully"
      assert html =~ "some room"
    end

    test "updates index in listing", %{conn: conn, index: index} do
      {:ok, index_live, _html} = live(conn, ~p"/game")

      assert {:ok, form_live, _html} =
               index_live
               |> element("#game-#{index.id} a", "Edit")
               |> render_click()
               |> follow_redirect(conn, ~p"/game/#{index}/edit")

      assert render(form_live) =~ "Edit Index"

      assert form_live
             |> form("#index-form", index: @invalid_attrs)
             |> render_change() =~ "can&#39;t be blank"

      assert {:ok, index_live, _html} =
               form_live
               |> form("#index-form", index: @update_attrs)
               |> render_submit()
               |> follow_redirect(conn, ~p"/game")

      html = render(index_live)
      assert html =~ "Index updated successfully"
      assert html =~ "some updated room"
    end

    test "deletes index in listing", %{conn: conn, index: index} do
      {:ok, index_live, _html} = live(conn, ~p"/game")

      assert index_live |> element("#game-#{index.id} a", "Delete") |> render_click()
      refute has_element?(index_live, "#game-#{index.id}")
    end
  end

  describe "Show" do
    setup [:create_index]

    test "displays index", %{conn: conn, index: index} do
      {:ok, _show_live, html} = live(conn, ~p"/game/#{index}")

      assert html =~ "Show Index"
      assert html =~ index.room
    end

    test "updates index and returns to show", %{conn: conn, index: index} do
      {:ok, show_live, _html} = live(conn, ~p"/game/#{index}")

      assert {:ok, form_live, _} =
               show_live
               |> element("a", "Edit")
               |> render_click()
               |> follow_redirect(conn, ~p"/game/#{index}/edit?return_to=show")

      assert render(form_live) =~ "Edit Index"

      assert form_live
             |> form("#index-form", index: @invalid_attrs)
             |> render_change() =~ "can&#39;t be blank"

      assert {:ok, show_live, _html} =
               form_live
               |> form("#index-form", index: @update_attrs)
               |> render_submit()
               |> follow_redirect(conn, ~p"/game/#{index}")

      html = render(show_live)
      assert html =~ "Index updated successfully"
      assert html =~ "some updated room"
    end
  end
end
