#include <doctest.h>

#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Scene.h"

using namespace Aquila;
using namespace Aquila::SceneManagement;

TEST_SUITE("Hierarchy") {
	TEST_CASE("destroying an entity destroys its whole branch") {
		Scene scene("Test");
		EntityManager &entities = *scene.get_entity_manager();
		Entity root = entities.create_entity("Root");
		Entity child = entities.create_entity("Child");
		Entity grandchild = entities.create_entity("Grandchild");
		Entity sibling = entities.create_entity("Sibling");
		entities.add_child(root, child);
		entities.add_child(child, grandchild);

		entities.destroy_entity(root);

		CHECK_FALSE(root.exists());
		CHECK_FALSE(child.exists());
		CHECK_FALSE(grandchild.exists());
		CHECK(sibling.exists());
	}

	TEST_CASE("destroying a child unlinks it from its parent") {
		Scene scene("Test");
		EntityManager &entities = *scene.get_entity_manager();
		Entity parent = entities.create_entity("Parent");
		Entity child = entities.create_entity("Child");
		entities.add_child(parent, child);

		entities.destroy_entity(child);

		CHECK(parent.exists());
		CHECK(entities.get_children(parent).empty());
	}

	TEST_CASE("queued entities are destroyed with their children on flush") {
		Scene scene("Test");
		EntityManager &entities = *scene.get_entity_manager();
		Entity parent = entities.create_entity("Parent");
		Entity child = entities.create_entity("Child");
		entities.add_child(parent, child);

		entities.queue_for_kill(parent);
		CHECK(parent.exists());
		entities.flush_deletion_queue();

		CHECK_FALSE(parent.exists());
		CHECK_FALSE(child.exists());
	}
}
