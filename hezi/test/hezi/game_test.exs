defmodule Hezi.GameTest do
  use Hezi.DataCase

  alias Hezi.Game

  describe "/game" do
    alias Hezi.Game.Index

    import Hezi.GameFixtures

    @invalid_attrs %{room: nil}

    test "list_/game/0 returns all /game" do
      index = index_fixture()
      assert Game.list_/game() == [index]
    end

    test "get_index!/1 returns the index with given id" do
      index = index_fixture()
      assert Game.get_index!(index.id) == index
    end

    test "create_index/1 with valid data creates a index" do
      valid_attrs = %{room: "some room"}

      assert {:ok, %Index{} = index} = Game.create_index(valid_attrs)
      assert index.room == "some room"
    end

    test "create_index/1 with invalid data returns error changeset" do
      assert {:error, %Ecto.Changeset{}} = Game.create_index(@invalid_attrs)
    end

    test "update_index/2 with valid data updates the index" do
      index = index_fixture()
      update_attrs = %{room: "some updated room"}

      assert {:ok, %Index{} = index} = Game.update_index(index, update_attrs)
      assert index.room == "some updated room"
    end

    test "update_index/2 with invalid data returns error changeset" do
      index = index_fixture()
      assert {:error, %Ecto.Changeset{}} = Game.update_index(index, @invalid_attrs)
      assert index == Game.get_index!(index.id)
    end

    test "delete_index/1 deletes the index" do
      index = index_fixture()
      assert {:ok, %Index{}} = Game.delete_index(index)
      assert_raise Ecto.NoResultsError, fn -> Game.get_index!(index.id) end
    end

    test "change_index/1 returns a index changeset" do
      index = index_fixture()
      assert %Ecto.Changeset{} = Game.change_index(index)
    end
  end

  describe "game" do
    alias Hezi.Game.Index

    import Hezi.GameFixtures

    @invalid_attrs %{room: nil}

    test "list_game/0 returns all game" do
      index = index_fixture()
      assert Game.list_game() == [index]
    end

    test "get_index!/1 returns the index with given id" do
      index = index_fixture()
      assert Game.get_index!(index.id) == index
    end

    test "create_index/1 with valid data creates a index" do
      valid_attrs = %{room: "some room"}

      assert {:ok, %Index{} = index} = Game.create_index(valid_attrs)
      assert index.room == "some room"
    end

    test "create_index/1 with invalid data returns error changeset" do
      assert {:error, %Ecto.Changeset{}} = Game.create_index(@invalid_attrs)
    end

    test "update_index/2 with valid data updates the index" do
      index = index_fixture()
      update_attrs = %{room: "some updated room"}

      assert {:ok, %Index{} = index} = Game.update_index(index, update_attrs)
      assert index.room == "some updated room"
    end

    test "update_index/2 with invalid data returns error changeset" do
      index = index_fixture()
      assert {:error, %Ecto.Changeset{}} = Game.update_index(index, @invalid_attrs)
      assert index == Game.get_index!(index.id)
    end

    test "delete_index/1 deletes the index" do
      index = index_fixture()
      assert {:ok, %Index{}} = Game.delete_index(index)
      assert_raise Ecto.NoResultsError, fn -> Game.get_index!(index.id) end
    end

    test "change_index/1 returns a index changeset" do
      index = index_fixture()
      assert %Ecto.Changeset{} = Game.change_index(index)
    end
  end
end
